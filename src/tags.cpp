#include "tags.h"
#include "m3u.h"
#include <windows.h>
#include "ma_config.h"
#include "miniaudio.h"
#include <algorithm>
#include <cstring>

namespace {
constexpr size_t kLimit = 4 * 1024 * 1024;
uint32_t BE(const uint8_t* p, int n) {
    uint32_t v = 0; while (n--) v = (v << 8) | *p++; return v;
}
uint32_t LE(const uint8_t* p) { return p[0] | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24); }
uint32_t Sync(const uint8_t* p) {
    return ((p[0] & 127u) << 21) | ((p[1] & 127u) << 14) | ((p[2] & 127u) << 7) | (p[3] & 127u);
}
std::wstring Clean(std::wstring s) {
    auto zero = s.find(L'\0'); if (zero != std::wstring::npos) s.resize(zero);
    while (!s.empty() && (s.back() == L' ' || s.back() == L'\r' || s.back() == L'\n')) s.pop_back();
    for (auto& c : s) if (c < L' ') c = L' ';
    return s;
}
std::wstring Latin(const uint8_t* p, size_t n) {
    std::wstring s; for (size_t i = 0; i < n; ++i) s.push_back(p[i]); return Clean(s);
}
std::vector<uint8_t> Unsync(const uint8_t* p, size_t n) {
    std::vector<uint8_t> out;
    for (size_t i = 0; i < n; ++i) {
        out.push_back(p[i]);
        if (p[i] == 255 && i + 1 < n && p[i + 1] == 0) ++i;
    }
    return out;
}
std::wstring Text(const uint8_t* p, size_t n) {
    if (!n) return {};
    uint8_t encoding = *p++; --n;
    if (!encoding) return Latin(p, n);
    if (encoding == 3) {
        std::wstring s;
        DecodeText(std::string(reinterpret_cast<const char*>(p), n), s);
        return Clean(s);
    }
    bool little = false;
    if (encoding == 1) {
        if (n < 2) return {};
        if (p[0] == 255 && p[1] == 254) little = true;
        else if (p[0] != 254 || p[1] != 255) return {};
        p += 2; n -= 2;
    } else if (encoding != 2) return {};
    std::wstring s;
    for (size_t i = 0; i + 1 < n; i += 2)
        s.push_back(static_cast<wchar_t>(little ? (p[i] | (p[i + 1] << 8)) : ((p[i] << 8) | p[i + 1])));
    return Clean(s);
}
void ID3(const std::vector<uint8_t>& h, TrackMetadata& tags) {
    if (h.size() < 10 || std::memcmp(h.data(), "ID3", 3)) return;
    int version = h[3];
    if (version < 2 || version > 4 || (version == 2 && (h[5] & 0x40))) return;
    size_t n = Sync(&h[6]);
    if (n > h.size() - 10) return;
    std::vector<uint8_t> bytes(h.begin() + 10, h.begin() + 10 + n);
    if (version < 4 && (h[5] & 0x80)) bytes = Unsync(bytes.data(), bytes.size());
    size_t offset = 0;
    if (version >= 3 && (h[5] & 0x40)) {
        if (bytes.size() < 4) return;
        offset = version == 4 ? Sync(bytes.data()) : size_t(BE(bytes.data(), 4)) + 4;
        if (offset < 4 || offset > bytes.size()) return;
    }
    size_t header = version == 2 ? 6 : 10;
    while (offset <= bytes.size() && bytes.size() - offset >= header) {
        const uint8_t* frame = bytes.data() + offset;
        if (!frame[0]) break;
        std::string id(reinterpret_cast<const char*>(frame), version == 2 ? 3 : 4);
        n = version == 2 ? BE(frame + 3, 3) : version == 3 ? BE(frame + 4, 4) : Sync(frame + 4);
        offset += header;
        if (n > bytes.size() - offset) break;
        const uint8_t* data = bytes.data() + offset;
        uint8_t flags = version == 2 ? 0 : frame[9];
        bool unsupported = version == 3 ? (flags & 0xe0) != 0 : version == 4 ? (flags & 0x4c) != 0 : false;
        if (!unsupported && (id == "TIT2" || id == "TT2" || id == "TPE1" || id == "TP1")) {
            auto value = version == 4 && ((flags & 2) || (h[5] & 0x80)) ? Unsync(data, n) : std::vector<uint8_t>(data, data + n);
            if (version == 4 && (flags & 1)) {
                if (value.size() >= 4) value.erase(value.begin(), value.begin() + 4); else value.clear();
            }
            std::wstring text = Text(value.data(), value.size());
            if (!text.empty()) {
                if (id == "TIT2" || id == "TT2") tags.title = text; else tags.artist = text;
            }
        }
        offset += n;
    }
}
void Comments(const uint8_t* p, size_t n, TrackMetadata& tags) {
    if (n < 4) return;
    size_t vendor = LE(p); p += 4; n -= 4;
    if (vendor > n || n - vendor < 4) return;
    p += vendor; n -= vendor;
    uint32_t count = LE(p); p += 4; n -= 4;
    for (uint32_t i = 0; i < count && n >= 4; ++i) {
        size_t len = LE(p); p += 4; n -= 4;
        if (len > n) return;
        std::string comment(reinterpret_cast<const char*>(p), len);
        auto equal = comment.find('=');
        if (equal != std::string::npos) {
            std::string key = comment.substr(0, equal);
            for (auto& c : key) if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
            std::wstring value;
            if (DecodeText(comment.substr(equal + 1), value)) {
                if (key == "TITLE" && tags.title.empty()) tags.title = Clean(value);
                if (key == "ARTIST" && tags.artist.empty()) tags.artist = Clean(value);
            }
        }
        p += len; n -= len;
    }
}
void Flac(const std::vector<uint8_t>& h, TrackMetadata& tags) {
    if (h.size() < 4 || std::memcmp(h.data(), "fLaC", 4)) return;
    size_t pos = 4;
    while (h.size() - pos >= 4) {
        uint8_t type = h[pos]; size_t n = BE(h.data() + pos + 1, 3); pos += 4;
        if (n > h.size() - pos) return;
        if ((type & 127) == 4) Comments(h.data() + pos, n, tags);
        pos += n;
        if (type & 128) break;
    }
}
void Ogg(const std::vector<uint8_t>& h, TrackMetadata& tags) {
    size_t pos = 0;
    uint32_t serial = 0; bool haveSerial = false;
    std::vector<uint8_t> packet;
    while (pos <= h.size() && h.size() - pos >= 27) {
        const uint8_t* page = h.data() + pos;
        if (std::memcmp(page, "OggS", 4) || page[4]) return;
        size_t count = page[26], header = 27 + count;
        if (header > h.size() - pos) return;
        size_t size = 0;
        for (size_t i = 0; i < count; ++i) size += page[27 + i];
        if (size > h.size() - pos - header) return;
        uint32_t pageSerial = LE(page + 14);
        if (!haveSerial) { serial = pageSerial; haveSerial = true; }
        if (pageSerial == serial) {
            if (!(page[5] & 1)) packet.clear();
            size_t cursor = pos + header;
            for (size_t i = 0; i < count; ++i) {
                size_t len = page[27 + i];
                if (packet.size() + len > kLimit) return;
                packet.insert(packet.end(), h.begin() + cursor, h.begin() + cursor + len);
                cursor += len;
                if (len < 255) {
                    if (packet.size() >= 7 && packet[0] == 3 && !std::memcmp(packet.data() + 1, "vorbis", 6)) {
                        Comments(packet.data() + 7, packet.size() - 7, tags); return;
                    }
                    packet.clear();
                }
            }
        }
        pos += header + size;
    }
}
}
TrackMetadata ParseTags(const std::vector<uint8_t>& head, const std::vector<uint8_t>& tail) {
    TrackMetadata tags;
    if (tail.size() >= 128) {
        const uint8_t* p = tail.data() + tail.size() - 128;
        if (!std::memcmp(p, "TAG", 3)) { tags.title = Latin(p + 3, 30); tags.artist = Latin(p + 33, 30); }
    }
    ID3(head, tags); Flac(head, tags); Ogg(head, tags);
    return tags;
}
TrackMetadata ReadTrackMetadata(const std::wstring& path) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return {};
    LARGE_INTEGER size{};
    std::vector<uint8_t> head, tail;
    DWORD read = 0;
    if (GetFileSizeEx(file, &size) && size.QuadPart >= 0) {
        head.resize(static_cast<size_t>(std::min<LONGLONG>(size.QuadPart, kLimit)));
        if (!ReadFile(file, head.data(), static_cast<DWORD>(head.size()), &read, nullptr)) read = 0;
        head.resize(read);
        if (size.QuadPart >= 128) {
            // Also retain the final Ogg page: the Unicode callback decoder uses
            // stb_vorbis push mode, whose length API returns zero.
            DWORD tailSize = static_cast<DWORD>(std::min<LONGLONG>(size.QuadPart, 65536));
            LARGE_INTEGER end; end.QuadPart = -static_cast<LONGLONG>(tailSize);
            if (SetFilePointerEx(file, end, nullptr, FILE_END)) {
                tail.resize(tailSize);
                if (!ReadFile(file, tail.data(), tailSize, &read, nullptr)) read = 0;
                tail.resize(read);
            }
        }
    }
    CloseHandle(file);
    TrackMetadata tags = ParseTags(head, tail);
    ma_decoder decoder;
    if (ma_decoder_init_file_w(path.c_str(), nullptr, &decoder) == MA_SUCCESS) {
        ma_uint64 frames = 0; ma_uint32 rate = 0;
        if (ma_decoder_get_data_format(&decoder, nullptr, nullptr, &rate, nullptr, 0) == MA_SUCCESS && rate &&
            ma_decoder_get_length_in_pcm_frames(&decoder, &frames) == MA_SUCCESS)
            tags.duration = static_cast<double>(frames) / rate;
        if (!frames && rate && head.size() >= 27 && !std::memcmp(head.data(), "OggS", 4)) {
            uint32_t serial = LE(head.data() + 14);
            for (size_t i = 0; i + 27 <= tail.size(); ++i) {
                const uint8_t* page = tail.data() + i;
                if (std::memcmp(page, "OggS", 4) || page[4] || !(page[5] & 4) || LE(page+14) != serial) continue;
                size_t segments = page[26], bytes = 0;
                if (i + 27 + segments > tail.size()) continue;
                for (size_t j = 0; j < segments; ++j) bytes += page[27+j];
                if (i + 27 + segments + bytes > tail.size()) continue;
                uint64_t granule = static_cast<uint64_t>(LE(page+6)) | (static_cast<uint64_t>(LE(page+10)) << 32);
                if (granule != UINT64_MAX) tags.duration = static_cast<double>(granule) / rate;
            }
        }
        tags.readable = true;
        ma_decoder_uninit(&decoder);
    }
    return tags;
}

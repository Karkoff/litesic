#pragma once
#include <string>
#include <vector>
#include <cstdint>
struct TrackMetadata {
    std::wstring title, artist;
    std::wstring album, genre, year, trackNumber;
    uint64_t fileBytes = 0;
    uint32_t sampleRate = 0, channels = 0;
    double duration = -1;
    bool readable = false;
};
// Bounded parsers accept truncated/malformed input without reading past buffers.
TrackMetadata ParseTags(const std::vector<uint8_t>& head,
                        const std::vector<uint8_t>& tail);
TrackMetadata ReadTrackMetadata(const std::wstring& path);

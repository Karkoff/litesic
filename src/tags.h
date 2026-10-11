#pragma once
#include <string>
#include <vector>
#include <cstdint>
struct TrackMetadata {
    std::wstring title, artist;
    double duration = -1;
    bool readable = false;
};
// Bounded parsers accept truncated/malformed input without reading past buffers.
TrackMetadata ParseTags(const std::vector<uint8_t>& head,
                        const std::vector<uint8_t>& tail);
TrackMetadata ReadTrackMetadata(const std::wstring& path);

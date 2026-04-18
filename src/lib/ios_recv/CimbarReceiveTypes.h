/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace cimbar::ios_recv {

constexpr int kDefaultModeValue = 68;

enum class SessionPhase {
    Idle,
    Searching,
    Detecting,
    Decoding,
    Reconstructing,
    Completed,
    Error,
};

struct ProgressSnapshot {
    SessionPhase phase = SessionPhase::Idle;
    bool recognized_frame = false;
    bool needs_sharpen = false;
    int extracted_bytes = 0;
    int64_t completed_file_id = 0;
    std::string status_message;
    std::vector<int> fountain_progress;
};

struct CompletedFile {
    uint32_t file_id = 0;
    std::string filename;
    std::vector<unsigned char> compressed_bytes;
};

} // namespace cimbar::ios_recv

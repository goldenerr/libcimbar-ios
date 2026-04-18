/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "CimbarReceiveTypes.h"

#include "encoder/Decoder.h"
#include "extractor/Extractor.h"

#include <vector>

namespace cimbar::ios_recv {

class CimbarReceiveSession {
public:
    CimbarReceiveSession();

    int configure_mode(int mode_value);
    int mode_value() const;
    void reset();
    const ProgressSnapshot& progress() const;
    ProgressSnapshot process_frame(const unsigned char* imgdata,
                                   unsigned width,
                                   unsigned height,
                                   int format,
                                   unsigned stride = 0);

private:
    static int canonical_mode_value(int mode_value);
    void refresh_decode_state();

    int _mode_value = kDefaultModeValue;
    ProgressSnapshot _progress;
    Extractor _extractor;
    Decoder _decoder;
    std::vector<unsigned char> _chunk_buffer;
};

} // namespace cimbar::ios_recv

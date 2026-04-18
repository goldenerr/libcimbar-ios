/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "CimbarReceiveSession.h"

#include "encoder/escrow_buffer_writer.h"
#include "cimb_translator/Config.h"

#include <opencv2/opencv.hpp>

namespace cimbar::ios_recv {

namespace {
constexpr int kLegacy4ColorModeValue = 4;
constexpr int kLegacy8ColorModeValue = 8;
constexpr int kMicroModeValue = 66;
constexpr int kMiniModeValue = 67;

unsigned fountain_chunks_per_frame() {
    return cimbar::Config::fountain_chunks_per_frame(cimbar::Config::bits_per_cell());
}

unsigned fountain_chunk_size() {
    return cimbar::Config::fountain_chunk_size();
}

cv::UMat get_rgb(const unsigned char* imgdata, unsigned width, unsigned height, int type, unsigned stride) {
    cv::UMat img;
    size_t row_stride = stride > 0 ? static_cast<size_t>(stride) : static_cast<size_t>(cv::Mat::AUTO_STEP);
    switch (type) {
        case 12:
            img = cv::Mat(height * 3 / 2, width, CV_8UC1, const_cast<unsigned char*>(imgdata), row_stride).getUMat(cv::ACCESS_READ).clone();
            cv::cvtColor(img, img, cv::COLOR_YUV2RGB_NV12);
            return img;
        case 420:
            img = cv::Mat(height * 3 / 2, width, CV_8UC1, const_cast<unsigned char*>(imgdata), row_stride).getUMat(cv::ACCESS_READ).clone();
            cv::cvtColor(img, img, cv::COLOR_YUV420p2RGB);
            return img;
        default:
            break;
    }

    int cvtype = type == 4 ? CV_8UC4 : CV_8UC3;
    img = cv::Mat(height, width, cvtype, const_cast<unsigned char*>(imgdata), row_stride).getUMat(cv::ACCESS_READ).clone();
    if (type == 4) {
        cv::cvtColor(img, img, cv::COLOR_RGBA2RGB);
    }
    return img;
}
} // namespace

int CimbarReceiveSession::canonical_mode_value(int mode_value) {
    switch (mode_value) {
        case kLegacy4ColorModeValue:
        case kLegacy8ColorModeValue:
        case kMicroModeValue:
        case kMiniModeValue:
        case kDefaultModeValue:
            return mode_value;
        default:
            return kDefaultModeValue;
    }
}

CimbarReceiveSession::CimbarReceiveSession() {
    refresh_decode_state();
}

int CimbarReceiveSession::configure_mode(int mode_value) {
    _mode_value = canonical_mode_value(mode_value);
    refresh_decode_state();
    reset();
    return 0;
}

int CimbarReceiveSession::mode_value() const {
    return _mode_value;
}

void CimbarReceiveSession::reset() {
    _progress = {};
}

const ProgressSnapshot& CimbarReceiveSession::progress() const {
    return _progress;
}

ProgressSnapshot CimbarReceiveSession::process_frame(const unsigned char* imgdata,
                                                     unsigned width,
                                                     unsigned height,
                                                     int format,
                                                     unsigned stride) {
    _progress = {};
    _progress.phase = SessionPhase::Searching;

    if (imgdata == nullptr || width == 0 || height == 0) {
        _progress.status_message = "invalid frame";
        return _progress;
    }

    if (_chunk_buffer.size() != fountain_chunks_per_frame() * fountain_chunk_size()) {
        refresh_decode_state();
    }

    escrow_buffer_writer ebw(_chunk_buffer.data(), fountain_chunks_per_frame(), fountain_chunk_size());
    cv::UMat img = get_rgb(imgdata, width, height, format <= 0 ? 3 : format, stride);

    bool should_preprocess = false;
    int extract_result = _extractor.extract(img, img);
    if (!extract_result) {
        _progress.status_message = "searching";
        return _progress;
    }

    _progress.recognized_frame = true;
    if (extract_result == Extractor::NEEDS_SHARPEN) {
        should_preprocess = true;
        _progress.needs_sharpen = true;
    }

    _decoder.decode_fountain(img, ebw, should_preprocess);
    _progress.extracted_bytes = static_cast<int>(ebw.buffers_in_use() * fountain_chunk_size());

    if (_progress.extracted_bytes > 0) {
        _progress.phase = SessionPhase::Decoding;
        _progress.status_message = "decoded frame chunks";
    } else {
        _progress.phase = SessionPhase::Detecting;
        _progress.status_message = "recognized frame without chunks";
    }

    return _progress;
}

void CimbarReceiveSession::refresh_decode_state() {
    cimbar::Config::update(_mode_value);
    _extractor = Extractor();
    _decoder = Decoder();
    _chunk_buffer.assign(fountain_chunks_per_frame() * fountain_chunk_size(), 0);
}

} // namespace cimbar::ios_recv

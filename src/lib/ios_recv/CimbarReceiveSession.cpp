/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "CimbarReceiveSession.h"

#include "cimb_translator/Config.h"

namespace cimbar::ios_recv {

namespace {
constexpr int kLegacy4ColorModeValue = 4;
constexpr int kLegacy8ColorModeValue = 8;
constexpr int kMicroModeValue = 66;
constexpr int kMiniModeValue = 67;
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
    cimbar::Config::update(_mode_value);
}

int CimbarReceiveSession::configure_mode(int mode_value) {
    _mode_value = canonical_mode_value(mode_value);
    cimbar::Config::update(_mode_value);
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

} // namespace cimbar::ios_recv

/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "CimbarReceiveTypes.h"

namespace cimbar::ios_recv {

class CimbarReceiveSession {
public:
    CimbarReceiveSession();

    int configure_mode(int mode_value);
    int mode_value() const;
    void reset();
    const ProgressSnapshot& progress() const;

private:
    static int canonical_mode_value(int mode_value);

    int _mode_value = kDefaultModeValue;
    ProgressSnapshot _progress;
};

} // namespace cimbar::ios_recv

/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "cimbar_ios_recv_c.h"

#include "CimbarReceiveSession.h"

struct cimbar_ios_recv_handle {
    cimbar::ios_recv::CimbarReceiveSession session;
};

namespace {

void fill_progress(const cimbar::ios_recv::ProgressSnapshot& progress, cimbar_ios_recv_progress* out_progress) {
    if (out_progress == nullptr) {
        return;
    }

    out_progress->phase = static_cast<int>(progress.phase);
    out_progress->recognized_frame = progress.recognized_frame ? 1 : 0;
    out_progress->needs_sharpen = progress.needs_sharpen ? 1 : 0;
    out_progress->extracted_bytes = progress.extracted_bytes;
    out_progress->completed_file_id = progress.completed_file_id;
}

} // namespace

extern "C" {

cimbar_ios_recv_handle* cimbar_ios_recv_create(void) {
    return new cimbar_ios_recv_handle();
}

void cimbar_ios_recv_destroy(cimbar_ios_recv_handle* handle) {
    delete handle;
}

int cimbar_ios_recv_reset(cimbar_ios_recv_handle* handle) {
    if (handle == nullptr) {
        return -1;
    }

    handle->session.reset();
    return 0;
}

int cimbar_ios_recv_configure_mode(cimbar_ios_recv_handle* handle, int mode_value) {
    if (handle == nullptr) {
        return -1;
    }

    return handle->session.configure_mode(mode_value);
}

int cimbar_ios_recv_process_frame(cimbar_ios_recv_handle* handle,
                                  const unsigned char* imgdata,
                                  unsigned width,
                                  unsigned height,
                                  int format,
                                  unsigned stride,
                                  cimbar_ios_recv_progress* out_progress) {
    if (handle == nullptr || out_progress == nullptr) {
        return -1;
    }

    fill_progress(handle->session.process_frame(imgdata, width, height, format, stride), out_progress);
    return 0;
}

} // extern "C"

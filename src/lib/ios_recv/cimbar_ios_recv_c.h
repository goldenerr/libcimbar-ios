/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cimbar_ios_recv_handle cimbar_ios_recv_handle;

typedef struct {
    int phase;
    int recognized_frame;
    int needs_sharpen;
    int extracted_bytes;
    int64_t completed_file_id;
} cimbar_ios_recv_progress;

cimbar_ios_recv_handle* cimbar_ios_recv_create(void);
void cimbar_ios_recv_destroy(cimbar_ios_recv_handle* handle);
int cimbar_ios_recv_reset(cimbar_ios_recv_handle* handle);
int cimbar_ios_recv_configure_mode(cimbar_ios_recv_handle* handle, int mode_value);
int cimbar_ios_recv_process_frame(cimbar_ios_recv_handle* handle,
                                  const unsigned char* imgdata,
                                  unsigned width,
                                  unsigned height,
                                  int format,
                                  unsigned stride,
                                  cimbar_ios_recv_progress* out_progress);

#ifdef __cplusplus
}
#endif

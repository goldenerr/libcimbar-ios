/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <stddef.h>
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
    int scanned_chunks;
    int total_chunks;
    char status_message[128];
} cimbar_ios_recv_progress;

typedef struct {
    uint32_t file_id;
    char* filename;
    unsigned char* data;
    size_t data_size;
} cimbar_ios_recv_completed_file;

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
int cimbar_ios_recv_take_completed_file(cimbar_ios_recv_handle* handle,
                                        cimbar_ios_recv_completed_file* out_file);
void cimbar_ios_recv_completed_file_release(cimbar_ios_recv_completed_file* file);

#ifdef __cplusplus
}
#endif

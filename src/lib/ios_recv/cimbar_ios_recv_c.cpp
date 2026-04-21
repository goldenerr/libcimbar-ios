/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "cimbar_ios_recv_c.h"

#include "CimbarReceiveSession.h"

#include <cstring>
#include <string>
#include <utility>
#include <vector>

struct cimbar_ios_recv_handle {
    cimbar::ios_recv::CimbarReceiveSession session;
    std::vector<cimbar::ios_recv::CompletedFile> completed_files;
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
    out_progress->scanned_chunks = progress.scanned_chunks;
    out_progress->total_chunks = progress.total_chunks;

    std::memset(out_progress->status_message, 0, sizeof(out_progress->status_message));
    if (!progress.status_message.empty()) {
        std::strncpy(out_progress->status_message,
                     progress.status_message.c_str(),
                     sizeof(out_progress->status_message) - 1);
    }
}

std::vector<unsigned char> pack_nv12_planes(const unsigned char* y_plane,
                                            unsigned y_stride,
                                            const unsigned char* uv_plane,
                                            unsigned uv_stride,
                                            unsigned width,
                                            unsigned height) {
    std::vector<unsigned char> packed(static_cast<size_t>(width) * static_cast<size_t>(height) * 3 / 2);
    unsigned char* dst = packed.data();

    for (unsigned row = 0; row < height; ++row) {
        std::memcpy(dst + static_cast<size_t>(row) * width,
                    y_plane + static_cast<size_t>(row) * y_stride,
                    width);
    }

    unsigned char* dst_uv = dst + static_cast<size_t>(width) * height;
    for (unsigned row = 0; row < height / 2; ++row) {
        std::memcpy(dst_uv + static_cast<size_t>(row) * width,
                    uv_plane + static_cast<size_t>(row) * uv_stride,
                    width);
    }

    return packed;
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
    handle->completed_files.clear();
    return 0;
}

int cimbar_ios_recv_configure_mode(cimbar_ios_recv_handle* handle, int mode_value) {
    if (handle == nullptr) {
        return -1;
    }

    handle->completed_files.clear();
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

    std::vector<cimbar::ios_recv::CompletedFile> completed_files = handle->session.take_completed_files();
    handle->completed_files.insert(
        handle->completed_files.end(),
        std::make_move_iterator(completed_files.begin()),
        std::make_move_iterator(completed_files.end())
    );
    return 0;
}

int cimbar_ios_recv_process_frame_nv12(cimbar_ios_recv_handle* handle,
                                       const unsigned char* y_plane,
                                       unsigned y_stride,
                                       const unsigned char* uv_plane,
                                       unsigned uv_stride,
                                       unsigned width,
                                       unsigned height,
                                       cimbar_ios_recv_progress* out_progress) {
    if (handle == nullptr || out_progress == nullptr || y_plane == nullptr || uv_plane == nullptr) {
        return -1;
    }

    std::vector<unsigned char> packed = pack_nv12_planes(y_plane, y_stride, uv_plane, uv_stride, width, height);
    return cimbar_ios_recv_process_frame(handle,
                                         packed.data(),
                                         width,
                                         height,
                                         12,
                                         width,
                                         out_progress);
}

int cimbar_ios_recv_take_completed_file(cimbar_ios_recv_handle* handle,
                                        cimbar_ios_recv_completed_file* out_file) {
    if (handle == nullptr || out_file == nullptr || handle->completed_files.empty()) {
        return -1;
    }

    cimbar::ios_recv::CompletedFile completed_file = std::move(handle->completed_files.front());
    handle->completed_files.erase(handle->completed_files.begin());

    out_file->file_id = completed_file.file_id;
    out_file->data_size = completed_file.decompressed_bytes.size();
    out_file->filename = nullptr;
    out_file->data = nullptr;

    std::string filename = completed_file.filename;
    out_file->filename = new char[filename.size() + 1];
    std::memcpy(out_file->filename, filename.c_str(), filename.size() + 1);

    if (out_file->data_size > 0) {
        out_file->data = new unsigned char[out_file->data_size];
        std::memcpy(out_file->data, completed_file.decompressed_bytes.data(), out_file->data_size);
    }

    return 0;
}

void cimbar_ios_recv_completed_file_release(cimbar_ios_recv_completed_file* file) {
    if (file == nullptr) {
        return;
    }

    delete[] file->filename;
    delete[] file->data;
    file->filename = nullptr;
    file->data = nullptr;
    file->data_size = 0;
    file->file_id = 0;
}

} // extern "C"

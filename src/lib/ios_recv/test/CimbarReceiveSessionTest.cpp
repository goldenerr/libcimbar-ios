/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"
#include "TestHelpers.h"
#include "ios_recv/CimbarReceiveSession.h"

#include <cstring>
#include <opencv2/opencv.hpp>
#include <vector>

extern "C" {
#include "ios_recv/cimbar_ios_recv_c.h"
}

namespace {

cv::Mat shift_channel(const cv::Mat& ch, int dx, int dy) {
    cv::Mat out;
    cv::Mat M = (cv::Mat_<double>(2, 3) << 1, 0, dx, 0, 1, dy);
    cv::warpAffine(ch, out, M, ch.size(), cv::INTER_LINEAR, cv::BORDER_REPLICATE);
    return out;
}

cv::Mat make_chroma_bleed_softened_frame(const cv::Mat& img,
                                        double scale = 0.92,
                                        int base_blur = 13,
                                        int green_blur = 9,
                                        int blue_blur = 17,
                                        int green_shift = 1,
                                        int blue_shift = 2) {
    cv::Mat softened;
    cv::resize(img, softened, cv::Size(), scale, scale, cv::INTER_LINEAR);
    cv::resize(softened, softened, img.size(), 0, 0, cv::INTER_LINEAR);
    cv::GaussianBlur(softened, softened, cv::Size(base_blur, base_blur), 0);

    std::vector<cv::Mat> ch;
    cv::split(softened, ch);
    cv::GaussianBlur(ch[1], ch[1], cv::Size(green_blur, green_blur), 0);
    cv::GaussianBlur(ch[2], ch[2], cv::Size(blue_blur, blue_blur), 0);
    ch[1] = shift_channel(ch[1], green_shift, 0);
    ch[2] = shift_channel(ch[2], blue_shift, 0);

    cv::Mat out;
    cv::merge(ch, out);
    return out;
}

std::vector<unsigned char> rgb_to_nv12(const cv::Mat& rgb) {
    cv::Mat yuv_i420;
    cv::cvtColor(rgb, yuv_i420, cv::COLOR_RGB2YUV_I420);

    const int width = rgb.cols;
    const int height = rgb.rows;
    const size_t y_plane_size = static_cast<size_t>(width) * static_cast<size_t>(height);
    const size_t chroma_plane_size = y_plane_size / 4;

    std::vector<unsigned char> nv12(y_plane_size + y_plane_size / 2);
    std::memcpy(nv12.data(), yuv_i420.data, y_plane_size);

    const unsigned char* u_plane = yuv_i420.data + y_plane_size;
    const unsigned char* v_plane = u_plane + chroma_plane_size;
    unsigned char* uv_plane = nv12.data() + y_plane_size;
    for (int row = 0; row < height / 2; ++row) {
        for (int col = 0; col < width / 2; ++col) {
            uv_plane[row * width + col * 2] = u_plane[row * (width / 2) + col];
            uv_plane[row * width + col * 2 + 1] = v_plane[row * (width / 2) + col];
        }
    }

    return nv12;
}

} // namespace

TEST_CASE("CimbarReceiveSession/defaultMode", "[unit]") {
    cimbar::ios_recv::CimbarReceiveSession session;
    assertEquals(68, session.mode_value());
}

TEST_CASE("CimbarReceiveSession/resetClearsProgress", "[unit]") {
    cimbar::ios_recv::CimbarReceiveSession session;
    session.configure_mode(4);
    session.reset();

    assertEquals(4, session.mode_value());
    assertEquals(static_cast<int>(cimbar::ios_recv::SessionPhase::Idle),
                 static_cast<int>(session.progress().phase));
}

TEST_CASE("cimbar_ios_recv_c/createResetDestroy", "[unit]") {
    auto* handle = cimbar_ios_recv_create();
    assertFalse(handle == nullptr);
    assertEquals(0, cimbar_ios_recv_reset(handle));
    cimbar_ios_recv_destroy(handle);
}

TEST_CASE("cimbar_ios_recv_c/processFrameReportsStatusMessage", "[unit]") {
    auto* handle = cimbar_ios_recv_create();
    assertFalse(handle == nullptr);

    cimbar_ios_recv_progress progress{};
    assertEquals(0, cimbar_ios_recv_process_frame(handle, nullptr, 0, 0, 3, 0, &progress));
    assertStringsEqual("invalid frame", progress.status_message);

    cimbar_ios_recv_destroy(handle);
}

TEST_CASE("CimbarReceiveSession/processFrameExtractsChunks", "[unit]") {
    cimbar::ios_recv::CimbarReceiveSession session;
    cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");

    cimbar::ios_recv::ProgressSnapshot progress = session.process_frame(img.data, img.cols, img.rows, 3, static_cast<unsigned>(img.step));

    assertTrue(progress.recognized_frame);
    assertTrue(progress.extracted_bytes > 0);
}

TEST_CASE("CimbarReceiveSession/processFrameCompletesFile", "[unit]") {
    cimbar::ios_recv::CimbarReceiveSession session;
    cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");

    cimbar::ios_recv::ProgressSnapshot progress = session.process_frame(img.data, img.cols, img.rows, 3, static_cast<unsigned>(img.step));
    std::vector<cimbar::ios_recv::CompletedFile> completed = session.take_completed_files();

    assertTrue(progress.recognized_frame);
    assertTrue(progress.extracted_bytes > 0);
    assertTrue(progress.completed_file_id > 0);
    assertEquals(static_cast<int>(cimbar::ios_recv::SessionPhase::Completed),
                 static_cast<int>(progress.phase));
    assertEquals(1, completed.size());
    assertTrue(!completed.front().filename.empty());
    assertTrue(completed.front().decompressed_bytes.size() > 0);
}

TEST_CASE("CimbarReceiveSession/processFrameReportsChunkCounts", "[unit]") {
    cimbar::ios_recv::CimbarReceiveSession session;
    cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");

    cimbar::ios_recv::ProgressSnapshot progress = session.process_frame(img.data, img.cols, img.rows, 3, static_cast<unsigned>(img.step));

    assertTrue(progress.total_chunks > 0);
    assertTrue(progress.scanned_chunks > 0);
    assertTrue(progress.scanned_chunks <= progress.total_chunks);
}

TEST_CASE("CimbarReceiveSession/processFrameRecoversSoftenedLockedFrameChunks", "[unit]") {
    cimbar::ios_recv::CimbarReceiveSession session;
    cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");

    cv::Mat softened;
    cv::resize(img, softened, cv::Size(), 0.92, 0.92, cv::INTER_LINEAR);
    cv::resize(softened, softened, img.size(), 0, 0, cv::INTER_LINEAR);
    cv::blur(softened, softened, cv::Size(9, 9));

    cimbar::ios_recv::ProgressSnapshot progress = session.process_frame(softened.data,
                                                                        softened.cols,
                                                                        softened.rows,
                                                                        3,
                                                                        static_cast<unsigned>(softened.step));

    assertTrue(progress.recognized_frame);
    assertTrue(progress.needs_sharpen);
    assertTrue(progress.extracted_bytes > 0);
    assertTrue(progress.scanned_chunks > 0);
    assertStringsEqual("decoded frame chunks after clarity fallback", progress.status_message);
}

TEST_CASE("CimbarReceiveSession/processFrameRecoversHarderSoftenedLockedFrameChunks", "[unit]") {
    cimbar::ios_recv::CimbarReceiveSession session;
    cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");

    cv::Mat softened;
    cv::resize(img, softened, cv::Size(), 0.76, 0.76, cv::INTER_LINEAR);
    cv::resize(softened, softened, img.size(), 0, 0, cv::INTER_LINEAR);
    cv::blur(softened, softened, cv::Size(7, 7));

    cimbar::ios_recv::ProgressSnapshot progress = session.process_frame(softened.data,
                                                                        softened.cols,
                                                                        softened.rows,
                                                                        3,
                                                                        static_cast<unsigned>(softened.step));

    assertTrue(progress.recognized_frame);
    assertTrue(progress.needs_sharpen);
    assertTrue(progress.extracted_bytes > 0);
    assertTrue(progress.scanned_chunks > 0);
    assertStringsEqual("decoded frame chunks after secondary clarity fallback", progress.status_message);
}

TEST_CASE("CimbarReceiveSession/processFrameDoesNotLeakColorCorrectionAcrossFrames", "[unit]") {
    cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");

    cv::Mat softened;
    cv::resize(img, softened, cv::Size(), 0.76, 0.76, cv::INTER_LINEAR);
    cv::resize(softened, softened, img.size(), 0, 0, cv::INTER_LINEAR);
    cv::blur(softened, softened, cv::Size(7, 7));

    cimbar::ios_recv::CimbarReceiveSession isolated_session;
    cimbar::ios_recv::ProgressSnapshot isolated = isolated_session.process_frame(softened.data,
                                                                                 softened.cols,
                                                                                 softened.rows,
                                                                                 3,
                                                                                 static_cast<unsigned>(softened.step));

    cimbar::ios_recv::CimbarReceiveSession clean_session;
    cimbar::ios_recv::ProgressSnapshot clean = clean_session.process_frame(img.data,
                                                                           img.cols,
                                                                           img.rows,
                                                                           3,
                                                                           static_cast<unsigned>(img.step));

    cimbar::ios_recv::CimbarReceiveSession after_clean_session;
    cimbar::ios_recv::ProgressSnapshot after_clean = after_clean_session.process_frame(softened.data,
                                                                                       softened.cols,
                                                                                       softened.rows,
                                                                                       3,
                                                                                       static_cast<unsigned>(softened.step));

    assertTrue(clean.completed_file_id > 0);
    assertStringsEqual("decoded frame chunks after secondary clarity fallback", isolated.status_message);
    assertStringsEqual(isolated.status_message, after_clean.status_message);
    assertEquals(isolated.extracted_bytes, after_clean.extracted_bytes);
}

TEST_CASE("CimbarReceiveSession/processFrameDecodesBlurredFrameAfterCorePreprocessTuning", "[unit]") {
    cimbar::ios_recv::CimbarReceiveSession session;
    cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");

    cv::Mat softened;
    cv::resize(img, softened, cv::Size(), 0.92, 0.92, cv::INTER_LINEAR);
    cv::resize(softened, softened, img.size(), 0, 0, cv::INTER_LINEAR);
    cv::GaussianBlur(softened, softened, cv::Size(17, 17), 0);

    cimbar::ios_recv::ProgressSnapshot progress = session.process_frame(softened.data,
                                                                        softened.cols,
                                                                        softened.rows,
                                                                        3,
                                                                        static_cast<unsigned>(softened.step));

    assertTrue(progress.recognized_frame);
    assertTrue(progress.needs_sharpen);
    assertTrue(progress.extracted_bytes >= 1875);
    assertTrue(progress.scanned_chunks > 0);
    assertStringsEqual("decoded frame chunks", progress.status_message);
}

TEST_CASE("CimbarReceiveSession/processFrameRecoversChromaBleedSoftenedLockedFrameChunks", "[unit]") {
    cimbar::ios_recv::CimbarReceiveSession session;
    cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");

    cv::Mat softened = make_chroma_bleed_softened_frame(img);

    cimbar::ios_recv::ProgressSnapshot progress = session.process_frame(softened.data,
                                                                        softened.cols,
                                                                        softened.rows,
                                                                        3,
                                                                        static_cast<unsigned>(softened.step));

    assertTrue(progress.recognized_frame);
    assertTrue(progress.needs_sharpen);
    assertTrue(progress.extracted_bytes > 0);
    assertTrue(progress.scanned_chunks > 0);
    assertStringsEqual("decoded frame chunks", progress.status_message);
}

TEST_CASE("CimbarReceiveSession/processFrameRecoversChromaBleedFourthTierBoundaryCase", "[unit]") {
    cimbar::ios_recv::CimbarReceiveSession session;
    cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");

    cv::Mat softened = make_chroma_bleed_softened_frame(img, 0.92, 13, 9, 17, 1, 3);

    cimbar::ios_recv::ProgressSnapshot progress = session.process_frame(softened.data,
                                                                        softened.cols,
                                                                        softened.rows,
                                                                        3,
                                                                        static_cast<unsigned>(softened.step));

    assertTrue(progress.recognized_frame);
    assertTrue(progress.needs_sharpen);
    assertTrue(progress.extracted_bytes > 0);
    assertTrue(progress.scanned_chunks > 0);
}

TEST_CASE("CimbarReceiveSession/processFrameRecoversStrongerChromaBleedFourthTierCase", "[unit]") {
    cimbar::ios_recv::CimbarReceiveSession session;
    cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");

    cv::Mat softened = make_chroma_bleed_softened_frame(img, 0.92, 13, 9, 19, 1, 3);

    cimbar::ios_recv::ProgressSnapshot progress = session.process_frame(softened.data,
                                                                        softened.cols,
                                                                        softened.rows,
                                                                        3,
                                                                        static_cast<unsigned>(softened.step));

    assertTrue(progress.recognized_frame);
    assertTrue(progress.needs_sharpen);
    assertTrue(progress.extracted_bytes > 0);
    assertTrue(progress.scanned_chunks > 0);
    assertStringsEqual("decoded frame chunks after fourth-tier fallback", progress.status_message);
}

TEST_CASE("CimbarReceiveSession/processFrameRecoversOffsetChromaBleedFourthTierCase", "[unit]") {
    cimbar::ios_recv::CimbarReceiveSession session;
    cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");

    cv::Mat softened = make_chroma_bleed_softened_frame(img, 0.92, 13, 9, 19, 2, 2);

    cimbar::ios_recv::ProgressSnapshot progress = session.process_frame(softened.data,
                                                                        softened.cols,
                                                                        softened.rows,
                                                                        3,
                                                                        static_cast<unsigned>(softened.step));

    assertTrue(progress.recognized_frame);
    assertTrue(progress.needs_sharpen);
    assertTrue(progress.extracted_bytes > 0);
    assertTrue(progress.scanned_chunks > 0);
    assertStringsEqual("decoded frame chunks after fourth-tier fallback", progress.status_message);
}

TEST_CASE("CimbarReceiveSession/processFrameRecoversMirroredChromaBleedFourthTierCase", "[unit]") {
    cimbar::ios_recv::CimbarReceiveSession session;
    cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");

    cv::Mat softened = make_chroma_bleed_softened_frame(img, 0.92, 13, 9, 19, -1, -3);

    cimbar::ios_recv::ProgressSnapshot progress = session.process_frame(softened.data,
                                                                        softened.cols,
                                                                        softened.rows,
                                                                        3,
                                                                        static_cast<unsigned>(softened.step));

    assertTrue(progress.recognized_frame);
    assertTrue(progress.needs_sharpen);
    assertTrue(progress.extracted_bytes > 0);
    assertTrue(progress.scanned_chunks > 0);
    assertStringsEqual("decoded frame chunks after fourth-tier fallback", progress.status_message);
}

TEST_CASE("CimbarReceiveSession/processFrameRecoversMirroredOffsetChromaBleedFourthTierCase", "[unit]") {
    cimbar::ios_recv::CimbarReceiveSession session;
    cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");

    cv::Mat softened = make_chroma_bleed_softened_frame(img, 0.92, 13, 9, 19, -2, -2);

    cimbar::ios_recv::ProgressSnapshot progress = session.process_frame(softened.data,
                                                                        softened.cols,
                                                                        softened.rows,
                                                                        3,
                                                                        static_cast<unsigned>(softened.step));

    assertTrue(progress.recognized_frame);
    assertTrue(progress.needs_sharpen);
    assertTrue(progress.extracted_bytes > 0);
    assertTrue(progress.scanned_chunks > 0);
    assertStringsEqual("decoded frame chunks after fourth-tier fallback", progress.status_message);
}

TEST_CASE("CimbarReceiveSession/processFrameRecoversExtremelySoftenedLockedFrameChunks", "[unit]") {
    cimbar::ios_recv::CimbarReceiveSession session;
    cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");

    cv::Mat softened;
    cv::resize(img, softened, cv::Size(), 0.88, 0.88, cv::INTER_LINEAR);
    cv::resize(softened, softened, img.size(), 0, 0, cv::INTER_LINEAR);
    cv::GaussianBlur(softened, softened, cv::Size(21, 21), 0);

    cimbar::ios_recv::ProgressSnapshot progress = session.process_frame(softened.data,
                                                                        softened.cols,
                                                                        softened.rows,
                                                                        3,
                                                                        static_cast<unsigned>(softened.step));

    assertTrue(progress.recognized_frame);
    assertTrue(progress.needs_sharpen);
    assertTrue(progress.extracted_bytes > 0);
    assertTrue(progress.scanned_chunks > 0);
    assertStringsEqual("decoded frame chunks after clarity fallback", progress.status_message);
}

TEST_CASE("cimbar_ios_recv_c/processNV12FrameCompletesCleanFrame", "[unit]") {
    auto* handle = cimbar_ios_recv_create();
    assertFalse(handle == nullptr);

    cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");
    std::vector<unsigned char> nv12 = rgb_to_nv12(img);
    const unsigned char* y_plane = nv12.data();
    const unsigned char* uv_plane = nv12.data() + (img.cols * img.rows);

    cimbar_ios_recv_progress progress{};
    assertEquals(0,
                 cimbar_ios_recv_process_frame_nv12(handle,
                                                    y_plane,
                                                    static_cast<unsigned>(img.cols),
                                                    uv_plane,
                                                    static_cast<unsigned>(img.cols),
                                                    static_cast<unsigned>(img.cols),
                                                    static_cast<unsigned>(img.rows),
                                                    &progress));

    assertTrue(progress.recognized_frame != 0);
    assertTrue(progress.extracted_bytes > 0);
    assertTrue(progress.completed_file_id > 0);
    assertStringsEqual("completed file", progress.status_message);

    cimbar_ios_recv_destroy(handle);
}

/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"
#include "TestHelpers.h"
#include "ios_recv/CimbarReceiveSession.h"

extern "C" {
#include "ios_recv/cimbar_ios_recv_c.h"
}

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

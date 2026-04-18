/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"
#include "TestHelpers.h"
#include "ios_recv/CimbarReceiveSession.h"

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

TEST_CASE("CimbarReceiveSession/processFrameExtractsChunks", "[unit]") {
    cimbar::ios_recv::CimbarReceiveSession session;
    cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");

    cimbar::ios_recv::ProgressSnapshot progress = session.process_frame(img.data, img.cols, img.rows, 3, static_cast<unsigned>(img.step));

    assertTrue(progress.recognized_frame);
    assertTrue(progress.extracted_bytes > 0);
}

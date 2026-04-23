/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"
#include "TestHelpers.h"

#include "ExtractorPlus.h"
#include "image_hash/average_hash.h"
#include "util/MakeTempDirectory.h"
#include <iostream>
#include <string>
#include <vector>

TEST_CASE( "ExtractorTest/testExtract", "[unit]" )
{
	MakeTempDirectory tempdir;

	std::string imgPath = tempdir.path() / "ex.jpg";
	ExtractorPlus ext(0, {1024, 1024}, 30);
	ext.extract(TestCimbar::getSample("6bit/4_30_f0_big.jpg"), imgPath);

	cv::Mat out = cv::imread(imgPath);
	cv::cvtColor(out, out, cv::COLOR_BGR2RGB);
	assertEquals( 0x2cab639cfa72624, image_hash::average_hash(out) );
}

TEST_CASE( "ExtractorTest/testExtractMid", "[unit]" )
{
	MakeTempDirectory tempdir;

	std::string imgPath = tempdir.path() / "ex.jpg";
	ExtractorPlus ext(0, {1024, 1024}, 30);
	ext.extract(TestCimbar::getSample("6bit/4_30_f2_734.jpg"), imgPath);

	cv::Mat out = cv::imread(imgPath);
	cv::cvtColor(out, out, cv::COLOR_BGR2RGB);
	assertEquals( 0xc7f8205e686bc02, image_hash::average_hash(out) );
}

TEST_CASE( "ExtractorTest/testExtractUpscale", "[unit]" )
{
	MakeTempDirectory tempdir;

	std::string imgPath = tempdir.path() / "exup.jpg";
	ExtractorPlus ext(0, {1024, 1024}, 30);
	ext.extract(TestCimbar::getSample("6bit/4_30_f0_627.jpg"), imgPath);

	cv::Mat out = cv::imread(imgPath);
	cv::cvtColor(out, out, cv::COLOR_BGR2RGB);
	assertEquals( 0x29c64eaca3356394, image_hash::average_hash(out) );
}

TEST_CASE( "ExtractorTest/testExtractSoftenedDisplayFrameFallsBackToAdaptiveScan", "[unit]" )
{
	cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");
	cv::Mat softened;
	cv::resize(img, softened, cv::Size(), 0.84, 0.84, cv::INTER_LINEAR);
	cv::resize(softened, softened, img.size(), 0, 0, cv::INTER_LINEAR);
	cv::GaussianBlur(softened, softened, cv::Size(9, 9), 0);

	cv::Mat out;
	Extractor ext(0, {1024, 1024}, 30);
	int result = ext.extract(softened, out);
	assertEquals( Extractor::NEEDS_SHARPEN, result );
	assertFalse( out.empty() );
}

static cv::Mat shift_channel(const cv::Mat& ch, int dx, int dy)
{
	cv::Mat out;
	cv::Mat M = (cv::Mat_<double>(2, 3) << 1, 0, dx, 0, 1, dy);
	cv::warpAffine(ch, out, M, ch.size(), cv::INTER_LINEAR, cv::BORDER_REPLICATE);
	return out;
}

TEST_CASE( "ExtractorTest/testExtractStrongChromaBleedFrameFallsBackToSharpenedAdaptiveScan", "[unit]" )
{
	cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");
	cv::Mat softened;
	cv::resize(img, softened, cv::Size(), 0.92, 0.92, cv::INTER_LINEAR);
	cv::resize(softened, softened, img.size(), 0, 0, cv::INTER_LINEAR);
	cv::GaussianBlur(softened, softened, cv::Size(13, 13), 0);

	std::vector<cv::Mat> ch;
	cv::split(softened, ch);
	cv::GaussianBlur(ch[1], ch[1], cv::Size(9, 9), 0);
	cv::GaussianBlur(ch[2], ch[2], cv::Size(17, 17), 0);
	ch[1] = shift_channel(ch[1], 1, 0);
	ch[2] = shift_channel(ch[2], 4, 0);
	cv::merge(ch, softened);

	cv::Mat out;
	Extractor ext(0, {1024, 1024}, 30);
	int result = ext.extract(softened, out);
	assertEquals( Extractor::NEEDS_SHARPEN, result );
	assertFalse( out.empty() );
}

TEST_CASE( "ExtractorTest/testExtractVerySoftChromaBleedFrameFallsBackToStrongerSharpenedAdaptiveScan", "[unit]" )
{
	cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");
	cv::Mat softened;
	cv::resize(img, softened, cv::Size(), 0.92, 0.92, cv::INTER_LINEAR);
	cv::resize(softened, softened, img.size(), 0, 0, cv::INTER_LINEAR);
	cv::GaussianBlur(softened, softened, cv::Size(15, 15), 0);

	std::vector<cv::Mat> ch;
	cv::split(softened, ch);
	cv::GaussianBlur(ch[1], ch[1], cv::Size(9, 9), 0);
	cv::GaussianBlur(ch[2], ch[2], cv::Size(17, 17), 0);
	ch[1] = shift_channel(ch[1], 1, 0);
	ch[2] = shift_channel(ch[2], 3, 0);
	cv::merge(ch, softened);

	cv::Mat out;
	Extractor ext(0, {1024, 1024}, 30);
	int result = ext.extract(softened, out);
	assertEquals( Extractor::NEEDS_SHARPEN, result );
	assertFalse( out.empty() );
}

TEST_CASE( "ExtractorTest/testExtractHeavierBlueBleedFrameFallsBackToThirdSharpenedAdaptiveScan", "[unit]" )
{
	cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");
	cv::Mat softened;
	cv::resize(img, softened, cv::Size(), 0.92, 0.92, cv::INTER_LINEAR);
	cv::resize(softened, softened, img.size(), 0, 0, cv::INTER_LINEAR);
	cv::GaussianBlur(softened, softened, cv::Size(15, 15), 0);

	std::vector<cv::Mat> ch;
	cv::split(softened, ch);
	cv::GaussianBlur(ch[1], ch[1], cv::Size(9, 9), 0);
	cv::GaussianBlur(ch[2], ch[2], cv::Size(17, 17), 0);
	ch[1] = shift_channel(ch[1], 1, 0);
	ch[2] = shift_channel(ch[2], 4, 0);
	cv::merge(ch, softened);

	cv::Mat out;
	Extractor ext(0, {1024, 1024}, 30);
	int result = ext.extract(softened, out);
	assertEquals( Extractor::NEEDS_SHARPEN, result );
	assertFalse( out.empty() );
}

TEST_CASE( "ExtractorTest/testExtractDoubleShiftBlueBleedFrameFallsBackToFourthSharpenedAdaptiveScan", "[unit]" )
{
	cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");
	cv::Mat softened;
	cv::resize(img, softened, cv::Size(), 0.92, 0.92, cv::INTER_LINEAR);
	cv::resize(softened, softened, img.size(), 0, 0, cv::INTER_LINEAR);
	cv::GaussianBlur(softened, softened, cv::Size(15, 15), 0);

	std::vector<cv::Mat> ch;
	cv::split(softened, ch);
	cv::GaussianBlur(ch[1], ch[1], cv::Size(9, 9), 0);
	cv::GaussianBlur(ch[2], ch[2], cv::Size(17, 17), 0);
	ch[1] = shift_channel(ch[1], 2, 0);
	ch[2] = shift_channel(ch[2], 4, 0);
	cv::merge(ch, softened);

	cv::Mat out;
	Extractor ext(0, {1024, 1024}, 30);
	int result = ext.extract(softened, out);
	assertEquals( Extractor::NEEDS_SHARPEN, result );
	assertFalse( out.empty() );
}

TEST_CASE( "ExtractorTest/testExtractHeavyGreenAndBlueBleedFrameFallsBackToFifthSharpenedAdaptiveScan", "[unit]" )
{
	cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");
	cv::Mat softened;
	cv::resize(img, softened, cv::Size(), 0.92, 0.92, cv::INTER_LINEAR);
	cv::resize(softened, softened, img.size(), 0, 0, cv::INTER_LINEAR);
	cv::GaussianBlur(softened, softened, cv::Size(15, 15), 0);

	std::vector<cv::Mat> ch;
	cv::split(softened, ch);
	cv::GaussianBlur(ch[1], ch[1], cv::Size(11, 11), 0);
	cv::GaussianBlur(ch[2], ch[2], cv::Size(17, 17), 0);
	ch[1] = shift_channel(ch[1], 2, 0);
	ch[2] = shift_channel(ch[2], 4, 0);
	cv::merge(ch, softened);

	cv::Mat out;
	Extractor ext(0, {1024, 1024}, 30);
	int result = ext.extract(softened, out);
	assertEquals( Extractor::NEEDS_SHARPEN, result );
	assertFalse( out.empty() );
}

TEST_CASE( "ExtractorTest/testExtractHeavyGreen13AndBlueBleedFrameFallsBackToCompoundSharpenedAdaptiveScan", "[unit]" )
{
	cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");
	cv::Mat softened;
	cv::resize(img, softened, cv::Size(), 0.92, 0.92, cv::INTER_LINEAR);
	cv::resize(softened, softened, img.size(), 0, 0, cv::INTER_LINEAR);
	cv::GaussianBlur(softened, softened, cv::Size(15, 15), 0);

	std::vector<cv::Mat> ch;
	cv::split(softened, ch);
	cv::GaussianBlur(ch[1], ch[1], cv::Size(13, 13), 0);
	cv::GaussianBlur(ch[2], ch[2], cv::Size(17, 17), 0);
	ch[1] = shift_channel(ch[1], 2, 0);
	ch[2] = shift_channel(ch[2], 4, 0);
	cv::merge(ch, softened);

	cv::Mat out;
	Extractor ext(0, {1024, 1024}, 30);
	int result = ext.extract(softened, out);
	assertEquals( Extractor::NEEDS_SHARPEN, result );
	assertFalse( out.empty() );
}


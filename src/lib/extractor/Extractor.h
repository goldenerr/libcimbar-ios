/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "Deskewer.h"
#include "Scanner.h"
#include "util/vec_xy.h"

#include <opencv2/opencv.hpp>
#include <vector>

class Extractor
{
public:
	static constexpr int FAILURE = 0;
	static constexpr int SUCCESS = 1;
	static constexpr int NEEDS_SHARPEN = 2;

public:
	Extractor(unsigned padding=0, cimbar::vec_xy image_size={}, unsigned anchor_size=0);

	template <typename MAT>
	int extract(const MAT& img, MAT& out);

protected:
	cimbar::vec_xy _imageSize;
	unsigned _anchorSize;
	unsigned _padding;
};

template <typename MAT>
inline int Extractor::extract(const MAT& img, MAT& out)
{
	auto scan_points = [] (const MAT& scan_img, bool fast) {
		Scanner scanner(scan_img, fast);
		return scanner.scan();
	};
	auto apply_resample_variant = [] (const MAT& src, double scale) {
		MAT downscaled;
		MAT restored;
		cv::resize(src, downscaled, cv::Size(), scale, scale, cv::INTER_AREA);
		cv::resize(downscaled, restored, src.size(), 0, 0, cv::INTER_LINEAR);
		return restored;
	};
	auto apply_channel_realign_variant = [] (const MAT& src, int green_dx, int blue_dx) {
		std::vector<MAT> channels;
		cv::split(src, channels);
		cv::Mat green_shift = (cv::Mat_<double>(2, 3) << 1, 0, green_dx, 0, 1, 0);
		cv::Mat blue_shift = (cv::Mat_<double>(2, 3) << 1, 0, blue_dx, 0, 1, 0);
		cv::warpAffine(channels[1], channels[1], green_shift, channels[1].size(), cv::INTER_LINEAR, cv::BORDER_REPLICATE);
		cv::warpAffine(channels[2], channels[2], blue_shift, channels[2].size(), cv::INTER_LINEAR, cv::BORDER_REPLICATE);
		MAT realigned;
		cv::merge(channels, realigned);
		return realigned;
	};
	auto enhanced_scan_points = [&] (const MAT& source) {
		auto try_unsharp = [&] (double sigma, double alpha, double beta) {
			MAT blurred;
			MAT sharpened;
			cv::GaussianBlur(source, blurred, cv::Size(0, 0), sigma);
			cv::addWeighted(source, alpha, blurred, beta, 0.0, sharpened);
			return scan_points(sharpened, false);
		};

		for (const auto& pass : std::vector<std::tuple<double, double, double>>{
			{1.0, 2.0, -1.0},
			{1.2, 2.4, -1.4},
			{1.6, 3.0, -2.0},
			{1.8, 3.2, -2.2},
			{2.2, 3.6, -2.6},
			{3.0, 4.4, -3.4}
		}) {
			auto found = try_unsharp(std::get<0>(pass), std::get<1>(pass), std::get<2>(pass));
			if (found.size() >= 4)
				return found;
		}

		MAT preconditioned;
		MAT preconditioned_blur;
		MAT compound_sharpened;
		MAT compound_blur;
		cv::GaussianBlur(source, preconditioned_blur, cv::Size(0, 0), 1.2);
		cv::addWeighted(source, 2.4, preconditioned_blur, -1.4, 0.0, preconditioned);
		cv::GaussianBlur(preconditioned, compound_blur, cv::Size(0, 0), 1.0);
		cv::addWeighted(preconditioned, 2.0, compound_blur, -1.0, 0.0, compound_sharpened);
		return scan_points(compound_sharpened, false);
	};

	bool used_enhanced_search = false;
	std::vector<Anchor> points = scan_points(img, true);
	if (points.size() < 4)
		points = scan_points(img, false);
	if (points.size() < 4) {
		points = enhanced_scan_points(img);
		used_enhanced_search = points.size() >= 4;
	}
	if (points.size() < 4) {
		MAT resampled_94 = apply_resample_variant(img, 0.94);
		MAT resampled_88 = apply_resample_variant(img, 0.88);
		MAT resampled_82 = apply_resample_variant(img, 0.82);
		std::vector<MAT> display_variants = {
			resampled_94,
			resampled_88,
			resampled_82,
			apply_channel_realign_variant(resampled_94, 1, 3),
			apply_channel_realign_variant(resampled_94, 2, 2),
			apply_channel_realign_variant(resampled_94, -1, -3),
			apply_channel_realign_variant(resampled_94, -2, -2),
			apply_channel_realign_variant(resampled_88, 1, 3),
			apply_channel_realign_variant(resampled_88, 2, 2),
			apply_channel_realign_variant(resampled_88, 2, 3),
			apply_channel_realign_variant(resampled_88, -1, -3),
			apply_channel_realign_variant(resampled_88, -2, -2),
			apply_channel_realign_variant(resampled_82, 2, 3),
			apply_channel_realign_variant(resampled_82, 3, 2),
			apply_channel_realign_variant(resampled_82, -2, -3),
			apply_channel_realign_variant(resampled_82, -3, -2)
		};
		for (const MAT& variant : display_variants) {
			points = scan_points(variant, true);
			if (points.size() < 4)
				points = scan_points(variant, false);
			if (points.size() < 4)
				points = enhanced_scan_points(variant);
			if (points.size() >= 4) {
				used_enhanced_search = true;
				break;
			}
		}
	}
	if (points.size() < 4)
		return FAILURE;

	Corners corners(points);
	Deskewer de(_padding, _imageSize, _anchorSize);
	out = de.deskew(img, corners);

	if (used_enhanced_search || !corners.is_granular_scale(_imageSize))
		return NEEDS_SHARPEN;
	return SUCCESS;
}

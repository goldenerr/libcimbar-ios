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

	std::vector<Anchor> points = scan_points(img, true);
	if (points.size() < 4)
		points = scan_points(img, false);
	if (points.size() < 4)
	{
		MAT sharpened;
		MAT blurred;
		cv::GaussianBlur(img, blurred, cv::Size(0, 0), 1.0);
		cv::addWeighted(img, 2.0, blurred, -1.0, 0.0, sharpened);
		points = scan_points(sharpened, false);
		if (points.size() < 4)
		{
			MAT stronger_sharpened;
			MAT stronger_blurred;
			cv::GaussianBlur(img, stronger_blurred, cv::Size(0, 0), 1.2);
			cv::addWeighted(img, 2.4, stronger_blurred, -1.4, 0.0, stronger_sharpened);
			points = scan_points(stronger_sharpened, false);
			if (points.size() < 4)
			{
				MAT strongest_sharpened;
				MAT strongest_blurred;
				cv::GaussianBlur(img, strongest_blurred, cv::Size(0, 0), 1.6);
				cv::addWeighted(img, 3.0, strongest_blurred, -2.0, 0.0, strongest_sharpened);
				points = scan_points(strongest_sharpened, false);
				if (points.size() < 4)
				{
					MAT extreme_sharpened;
					MAT extreme_blurred;
					cv::GaussianBlur(img, extreme_blurred, cv::Size(0, 0), 1.8);
					cv::addWeighted(img, 3.2, extreme_blurred, -2.2, 0.0, extreme_sharpened);
					points = scan_points(extreme_sharpened, false);
					if (points.size() < 4)
					{
						MAT maximal_sharpened;
						MAT maximal_blurred;
						cv::GaussianBlur(img, maximal_blurred, cv::Size(0, 0), 2.2);
						cv::addWeighted(img, 3.6, maximal_blurred, -2.6, 0.0, maximal_sharpened);
						points = scan_points(maximal_sharpened, false);
						if (points.size() < 4)
						{
							MAT preconditioned;
							MAT preconditioned_blur;
							MAT compound_sharpened;
							MAT compound_blur;
							cv::GaussianBlur(img, preconditioned_blur, cv::Size(0, 0), 1.2);
							cv::addWeighted(img, 2.4, preconditioned_blur, -1.4, 0.0, preconditioned);
							cv::GaussianBlur(preconditioned, compound_blur, cv::Size(0, 0), 1.0);
							cv::addWeighted(preconditioned, 2.0, compound_blur, -1.0, 0.0, compound_sharpened);
							points = scan_points(compound_sharpened, false);
							if (points.size() < 4)
							{
								MAT severe_sharpened;
								MAT severe_blurred;
								cv::GaussianBlur(img, severe_blurred, cv::Size(0, 0), 3.0);
								cv::addWeighted(img, 4.4, severe_blurred, -3.4, 0.0, severe_sharpened);
								points = scan_points(severe_sharpened, false);
							}
						}
					}
				}
			}
		}
	}
	if (points.size() < 4)
		return FAILURE;

	Corners corners(points);
	Deskewer de(_padding, _imageSize, _anchorSize);
	out = de.deskew(img, corners);

	if ( !corners.is_granular_scale(_imageSize) )
		return NEEDS_SHARPEN;
	return SUCCESS;
}

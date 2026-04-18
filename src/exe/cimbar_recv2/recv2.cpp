/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "gui/window_glfw.h"
#include "ios_recv/CimbarReceiveSession.h"

#include "cxxopts/cxxopts.hpp"
#include "serialize/format.h"
#include "serialize/str.h"

#include <GLFW/glfw3.h>
#include <opencv2/videoio.hpp>

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
using std::string;

namespace {

	template <typename TP>
	TP wait_for_frame_time(unsigned delay, const TP& start)
	{
		unsigned millis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();
		if (delay > millis)
			std::this_thread::sleep_for(std::chrono::milliseconds(delay-millis));
		return std::chrono::high_resolution_clock::now();
	}
}


int main(int argc, char** argv)
{
	cxxopts::Options options("cimbar video decoder", "Use the camera to decode data!");

	unsigned defaultFps = 15;
	options.add_options()
		("i,in", "Video source.", cxxopts::value<string>())
		("o,out", "Output directory (decoding).", cxxopts::value<string>())
		("f,fps", "Target decode FPS", cxxopts::value<unsigned>()->default_value(turbo::str::str(defaultFps)))
		("m,mode", "Select a cimbar mode. B (the default) is new to 0.6.x. 4C is the 0.5.x config. [B,Bm,Bu,4C]", cxxopts::value<string>()->default_value("B"))
		("h,help", "Print usage")
	;
	options.show_positional_help();
	options.parse_positional({"in", "out"});
	options.positional_help("<in> <out>");

	auto result = options.parse(argc, argv);
	if (result.count("help") or !result.count("in") or !result.count("out"))
	{
	  std::cout << options.help() << std::endl;
	  return 0;
	}

	string source = result["in"].as<string>();
	string outpath = result["out"].as<string>();

	unsigned config_mode = 68;
	if (result.count("mode"))
	{
		string mode = result["mode"].as<string>();
		if (mode == "4c" or mode == "4C")
			config_mode = 4;
		else if (mode == "Bu" or mode == "BU")
			config_mode = 66;
		else if (mode == "Bm" or mode == "BM")
			config_mode = 67;
	}

	unsigned fps = result["fps"].as<unsigned>();
	if (fps == 0)
		fps = defaultFps;
	unsigned delay = 1000 / fps;

	cv::VideoCapture vc(source.c_str());
	if (!vc.isOpened())
	{
		std::cerr << "failed to open video device :(" << std::endl;
		return 70;
	}
	vc.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
	vc.set(cv::CAP_PROP_FRAME_HEIGHT, 1200);
	vc.set(cv::CAP_PROP_FPS, fps);

	// set max camera res, and use aspect ratio for window size...

	std::cout << fmt::format("width: {}, height {}, exposure {}, fps {}", vc.get(cv::CAP_PROP_FRAME_WIDTH), vc.get(cv::CAP_PROP_FRAME_HEIGHT), vc.get(cv::CAP_PROP_EXPOSURE), vc.get(cv::CAP_PROP_FPS)) << std::endl;

	double ratio = vc.get(cv::CAP_PROP_FRAME_WIDTH) / vc.get(cv::CAP_PROP_FRAME_HEIGHT);
	int height = 600;
	int width = height * ratio;
	std::cout << "got dimensions " << width << "," << height << std::endl;

	cimbar::window_glfw window(width, height, "cimbar_recv");
	if (!window.is_good())
	{
		std::cerr << "failed to create window :(" << std::endl;
		return 70;
	}
	window.auto_scale_to_window();

	// allocate buffers, etc
	cimbar::ios_recv::CimbarReceiveSession session;
	session.configure_mode(config_mode);

	cv::Mat mat;

	unsigned count = 0;
	std::chrono::time_point start = std::chrono::high_resolution_clock::now();
	while (true)
	{
		++count;

		// delay, then try to read frame
		start = wait_for_frame_time(delay, start);
		if (window.should_close())
			break;

		if (!vc.read(mat))
		{
			std::cerr << "failed to read from cam" << std::endl;
			continue;
		}

		cv::Mat img = mat.clone();
		cv::cvtColor(mat, mat, cv::COLOR_BGR2RGB);

		// draw some stats on mat?
		window.show(mat, 0);

		// route frame decode/reconstruction through the reusable session abstraction
		auto snapshot = session.process_frame(img.data,
		                                    static_cast<unsigned>(img.cols),
		                                    static_cast<unsigned>(img.rows),
		                                    img.channels(),
		                                    static_cast<unsigned>(img.step));
		(void)snapshot;

		for (auto& file : session.take_completed_files())
		{
			std::cerr << "Saving file " << file.filename << " of (decompressed) size " << file.decompressed_bytes.size() << std::endl;

			std::string file_path = fmt::format("{}/{}", outpath, file.filename);
			std::ofstream outs(file_path, std::ios::binary);
			outs.write(reinterpret_cast<const char*>(file.decompressed_bytes.data()), file.decompressed_bytes.size());
		}
	}

	return 0;
}

#include "encoder/EncoderPlus.h"
#include "cimb_translator/Config.h"
#include "serialize/format.h"
#include <opencv2/opencv.hpp>
#include <fstream>
#include <cstdio>

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input_file> <output_prefix> [mode=68] [compression=16] [redundancy=4.0]\n", argv[0]);
        return 1;
    }

    const char* input = argv[1];
    const char* prefix = argv[2];
    int mode = argc > 3 ? atoi(argv[3]) : 68;
    int compression = argc > 4 ? atoi(argv[4]) : 16;
    double redundancy = argc > 5 ? atof(argv[5]) : 4.0;

    cimbar::Config::update(mode);

    std::ifstream f(input, std::ios::binary);
    if (!f) {
        fprintf(stderr, "ERROR: cannot open %s\n", input);
        return 1;
    }

    EncoderPlus encoder;
    encoder.set_encode_id(0);

    unsigned count = 0;
    auto on_frame = [&](const cv::Mat& frame, unsigned i) -> bool {
        std::string path = fmt::format("{}_{:04d}.png", prefix, i);
        cv::Mat bgr;
        cv::cvtColor(frame, bgr, cv::COLOR_RGB2BGR);
        if (!cv::imwrite(path, bgr)) {
            fprintf(stderr, "ERROR: cannot write %s\n", path.c_str());
            return false;
        }
        count++;
        if (count % 100 == 0)
            fprintf(stderr, "  frame %u...\n", count);
        return true;
    };

    fprintf(stderr, "Encoding %s -> %s_*.png (mode=%d, redundancy=%.1fx)...\n", input, prefix, mode, redundancy);
    unsigned frames = encoder.encode_fountain(input, on_frame, compression, redundancy);
    fprintf(stderr, "Done: %u frames\n", frames);
    return frames > 0 ? 0 : 1;
}

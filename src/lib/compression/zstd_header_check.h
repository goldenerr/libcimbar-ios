/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <cstdint>
#include <string>

namespace cimbar {

class zstd_header_check
{
public:
	static std::string get_filename(const unsigned char* data, size_t len)
	{
		if (data == nullptr || len < 9) {
			return "";
		}

		constexpr uint32_t kSkippableStart = 0x184D2A50u;
		uint32_t magic = static_cast<uint32_t>(data[0])
			| (static_cast<uint32_t>(data[1]) << 8)
			| (static_cast<uint32_t>(data[2]) << 16)
			| (static_cast<uint32_t>(data[3]) << 24);
		if (magic < kSkippableStart || magic > kSkippableStart + 15u) {
			return "";
		}

		uint32_t payloadSize = static_cast<uint32_t>(data[4])
			| (static_cast<uint32_t>(data[5]) << 8)
			| (static_cast<uint32_t>(data[6]) << 16)
			| (static_cast<uint32_t>(data[7]) << 24);
		constexpr size_t kHeaderSize = 8;
		if (len < kHeaderSize + payloadSize || payloadSize <= 1) {
			return "";
		}

		const unsigned char* payload = data + kHeaderSize;
		switch (payload[0])
		{
			case 1:
				return std::string(reinterpret_cast<const char*>(payload + 1), payloadSize - 1);
			default:
				break;
		}
		return "";
	}

};

}

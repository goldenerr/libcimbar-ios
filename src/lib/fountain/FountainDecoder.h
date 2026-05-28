/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "lt.h"
#include <optional>
#include <set>
#include <vector>

class FountainDecoder
{
public:
	FountainDecoder(size_t length, size_t packet_size)
	    : _length(length)
	    , _decoder(length, packet_size)
	{
	}

	unsigned progress() const
	{
		return _seenBlocks.size();
	}

	size_t length() const
	{
		return _length;
	}

	bool good() const
	{
		return _decoder.good();
	}

	bool decode(unsigned block_num, uint8_t* data, size_t length)
	{
		auto pear = _seenBlocks.insert(block_num);
		if (!pear.second)
			return false;

		return _decoder.decode(block_num, data, length);
	}

	bool recover(unsigned char* data, unsigned size)
	{
		return _decoder.recover(data, size);
	}

	std::optional<std::vector<uint8_t>> recover()
	{
		std::vector<uint8_t> bytes;
		bytes.resize(_length);
		if (!recover(bytes.data(), bytes.size()))
			return std::nullopt;

		return bytes;
	}

protected:
	LTDecoder _decoder;
	size_t _length;
	std::set<unsigned> _seenBlocks;
};

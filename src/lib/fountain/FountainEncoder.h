/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "lt.h"

class FountainEncoder
{
protected:
	void swap(FountainEncoder& other) throw()
	{
		std::swap(_encoder, other._encoder);
		std::swap(_packetSize, other._packetSize);
	}

public:
	FountainEncoder(const uint8_t* data, size_t length, size_t packet_size)
	    : _packetSize(packet_size)
	    , _encoder(data, length, packet_size)
	{
	}

	FountainEncoder& operator=(FountainEncoder temp)
	{
		temp.swap(*this);
		return *this;
	}

	bool good() const
	{
		return _encoder.good();
	}

	size_t encode(unsigned block_num, uint8_t* buff, size_t size)
	{
		return _encoder.encode(block_num, buff, size);
	}

	size_t packet_size() const
	{
		return _packetSize;
	}

protected:
	LTEncoder _encoder;
	size_t _packetSize;
};

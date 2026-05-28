/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "FountainDecoder.h"
#include "FountainEncoder.h"
#include "serialize/format.h"

#include "base91/base.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using std::string;
using namespace std;

TEST_CASE( "FountainEncodingTest/testEncoder", "[unit]" )
{
	std::string content;
	for (int i = 0; i < 1000; ++i)
		content += "0123456789";

	FountainEncoder encoder((uint8_t*)content.data(), content.size(), 1120);
	assertTrue( encoder.good() );

	std::array<uint8_t,1120> buff;
	for (int i = 0; i < 500; ++i)
		DYNAMIC_SECTION( i << ":" )
		{
			// LT codec: all blocks are fountain-encoded, same size
			assertEquals( 1120, encoder.encode(i, buff.data(), buff.size()) );
		}
}

TEST_CASE( "FountainEncodingTest/testEncodingAndDecoding", "[unit]" )
{
	static const unsigned packetSize = 1400;

	unsigned messageSize = 10000;
	std::string message;
	while (message.size() < messageSize)
		message += "0123456789";
	message.resize(messageSize);

	FountainEncoder encoder((uint8_t*)message.data(), message.size(), packetSize);
	assertTrue( encoder.good() );

	FountainDecoder decoder(message.size(), packetSize);
	assertTrue( decoder.good() );

	std::array<uint8_t,packetSize> block;
	int block_id = 0;
	unsigned decoded_blocks = 0;
	for (; block_id < 50; ++block_id)
	{
		if (block_id % 3 == 0)  // 33% packet loss
			continue;
		++decoded_blocks;

		encoder.encode(block_id, block.data(), block.size());
		if (not decoder.decode(block_id, block.data(), packetSize))
			continue;

		// decode returned true — recovery complete
		std::optional<vector<uint8_t>> res = decoder.recover();
		assertTrue( res );

		vector<uint8_t>& out = *res;
		vector<uint8_t> expected = std::vector<uint8_t>(message.begin(), message.end());
		assertEquals( messageSize, out.size() );
		assertEquals( expected, out );
		break;
	}

	// LT decoder: N=8 source blocks, needs ~N+epsilon blocks
	// With 33% loss, we get ~2/3 of blocks = ~11 unique to get 8 decoded
	assertTrue( block_id > 0 );
	assertTrue( decoded_blocks >= 8 );
	assertEquals( decoded_blocks, decoder.progress() );
}

TEST_CASE( "FountainEncodingTest/testConsistency", "[unit]" )
{
	static const unsigned packetSize = 626;

	unsigned messageSize = 1000;
	std::string message;
	while (message.size() < messageSize)
		message += "0123456789";
	message.resize(messageSize);

	// make sure encoding is deterministic
	FountainEncoder encoder((uint8_t*)message.data(), message.size(), packetSize);
	std::array<uint8_t,packetSize> block;
	std::vector<string> encoded;

	for (unsigned block_id = 0; block_id < 3; ++block_id)
	{
		unsigned bites = encoder.encode(block_id, block.data(), block.size());
		encoded.push_back(base91::encode(std::string((char*)block.data(), bites)));
	}

	// Verify encoding is deterministic: re-encode should give same results
	for (unsigned block_id = 0; block_id < 3; ++block_id)
	{
		unsigned bites = encoder.encode(block_id, block.data(), block.size());
		std::string actual = base91::encode(std::string((char*)block.data(), bites));
		assertEquals( encoded[block_id], actual );
	}

	// Verify decoding round-trips
	FountainDecoder decoder(messageSize, packetSize);
	bool recovered = false;
	for (unsigned block_id = 0; block_id < 20; ++block_id)
	{
		if (block_id == 1)
			continue; // simulate loss

		unsigned bites = encoder.encode(block_id, block.data(), block.size());
		bool res = decoder.decode(block_id, block.data(), bites);
		if (res)
		{
			std::optional<vector<uint8_t>> reassembled = decoder.recover();
			assertTrue( reassembled );
			std::string actual = std::string((char*)reassembled->data(), reassembled->size());
			assertEquals( message, actual );
			recovered = true;
			break;
		}
	}
	assertTrue( recovered );
}

TEST_CASE( "FountainEncodingTest/testWhichN", "[unit]" )
{
	static const unsigned packetSize = 624;

	unsigned messageSize = 6000;
	std::string message;
	while (message.size() < messageSize)
		message += "0123456789";
	message.resize(messageSize);

	// create encoder and decoder
	FountainEncoder encoder((uint8_t*)message.data(), message.size(), packetSize);
	FountainDecoder decoder(messageSize, packetSize);

	std::array<uint8_t,packetSize> block;
	std::vector<uint8_t> final_result;

	// decode with high block IDs, never using the "non-encoded" range
	int block_id = 105;
	for (; block_id >= 0; block_id -= 1)
	{
		unsigned bites = encoder.encode(block_id, block.data(), block.size());
		bool res = decoder.decode(block_id, block.data(), bites);
		if (res)
		{
			std::optional<vector<uint8_t>> reassembled = decoder.recover();
			assertTrue( reassembled );
			final_result = *reassembled;
			break;
		}
	}

	assertTrue( block_id >= 0 );
	assertEquals( message, string((char*)final_result.data(), final_result.size()) );
}

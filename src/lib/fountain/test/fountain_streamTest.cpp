/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "fountain_encoder_stream.h"
#include "fountain_decoder_stream.h"

#include "serialize/format.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using std::string;
using namespace std;

TEST_CASE( "FountainStreamTest/testEncoder", "[unit]" )
{
	stringstream input;
	for (int i = 0; i < 1000; ++i)
		input << "0123456789";

	fountain_encoder_stream::ptr fes = fountain_encoder_stream::create(input, 830);

	assertEquals( 0, fes->block_count() );
	assertEquals( 13, fes->blocks_required() );
	assertTrue( fes->good() );

	std::array<char, 140> buff;
	for (int i = 0; i < 1000; ++i)
	{
		unsigned res = fes->readsome(buff.data(), buff.size());
		assertEquals( res, buff.size() );
	}

	// LT doesn't double-encode the last block (wirehair did, hence 170→169)
	assertEquals( 169, fes->block_count() );
	assertEquals( 13, fes->blocks_required() );
	assertTrue( fes->good() );
}

TEST_CASE( "FountainStreamTest/testEncoder_BlockHeader", "[unit]" )
{
	stringstream input;
	for (int i = 0; i < 1000; ++i)
		input << "0123456789";

	fountain_encoder_stream::ptr fes = fountain_encoder_stream::create(input, 636);

	assertEquals( 0, fes->block_count() );
	assertEquals( 16, fes->blocks_required() );
	assertTrue( fes->good() );

	std::array<char, 636> buff;
	for (unsigned i = 0; i < 20; ++i)
	{
		unsigned res = fes->readsome(buff.data(), buff.size());
		assertEquals( res, buff.size() );

		// encode_id
		assertEquals( 0, buff[0] );

		// total size == 10000
		assertEquals( 0, buff[1] );
		assertEquals( 39, (unsigned)buff[2] );
		assertEquals( 16, buff[3] );

		// block_id (bytes 4-7, 32-bit) — LT always increments by 1 per block
		assertEquals( 0, buff[4] );
		assertEquals( 0, buff[5] );
		assertEquals( 0, buff[6] );
		assertEquals( i, (unsigned)buff[7] );
	}

	// LT doesn't double-encode last block: 20 reads → 20 blocks
	assertEquals( 20, fes->block_count() );
	assertEquals( 16, fes->blocks_required() );
	assertTrue( fes->good() );
}

TEST_CASE( "FountainStreamTest/testEncoder_DifferentBuffSizes", "[unit]" )
{
	stringstream input;
	for (int i = 0; i < 1000; ++i)
		input << "0123456789";

	stringstream input2;
	input2 << input.str();

	fountain_encoder_stream::ptr fes1 = fountain_encoder_stream::create(input, 830);
	fountain_encoder_stream::ptr fes2 = fountain_encoder_stream::create(input2, 830);

	stringstream oneforty;
	std::array<char, 140> buff1;
	for (int i = 0; i < 83; ++i)
	{
		unsigned res = fes1->readsome(buff1.data(), buff1.size());
		assertEquals( res, buff1.size() );
		oneforty << string(buff1.begin(), buff1.end());
	}

	stringstream full;
	std::array<char, 830> buff2;
	for (int i = 0; i < 14; ++i)
	{
		unsigned res = fes2->readsome(buff2.data(), buff2.size());
		assertEquals( res, buff2.size() );
		full << string(buff2.begin(), buff2.end());
	}

	assertEquals( full.str(), oneforty.str() );

	// LT: one fewer block (no double-encode)
	assertEquals( 14, fes1->block_count() );
	assertEquals( 14, fes2->block_count() );
}


TEST_CASE( "FountainStreamTest/testEncoder_Consistency", "[unit]" )
{
	stringstream input;
	for (int i = 0; i < 100; ++i)
		input << "0123456789";

	fountain_encoder_stream::ptr fes = fountain_encoder_stream::create(input, 400);

	// LT encodes all blocks (no raw unencoded pass-through like wirehair).
	// Test determinism: same block_num → same output every time.
	std::array<char, 400> buff1, buff2;
	unsigned res1 = fes->readsome(buff1.data(), buff1.size());
	assertEquals( res1, buff1.size() );

	// Re-create encoder and encode same first block
	stringstream input2;
	for (int i = 0; i < 100; ++i)
		input2 << "0123456789";
	fountain_encoder_stream::ptr fes2 = fountain_encoder_stream::create(input2, 400);
	unsigned res2 = fes2->readsome(buff2.data(), buff2.size());
	assertEquals( res2, buff2.size() );

	// Same data → same block output
	assertEquals( string(buff1.begin(), buff1.end()), string(buff2.begin(), buff2.end()) );
}

TEST_CASE( "FountainStreamTest/testEncoder_ChangeBufferSize", "[unit]" )
{
	stringstream input;
	for (int i = 0; i < 1000; ++i)
		input << "0123456789";

	fountain_encoder_stream::ptr fes = fountain_encoder_stream::create(input, 830);
	assertEquals( 0, fes->block_count() );
	assertEquals( 13, fes->blocks_required() );
	assertTrue( fes->good() );

	assertTrue( fes->restart_and_resize_buffer(600) );

	// changes block params (and internal buffer size)
	assertEquals( 0, fes->block_count() );
	assertEquals( 17, fes->blocks_required() );
	assertTrue( fes->good() );

	std::array<char, 140> buff;
	for (int i = 0; i < 1000; ++i)
	{
		unsigned res = fes->readsome(buff.data(), buff.size());
		assertEquals( res, buff.size() );
	}

	// LT: one fewer block
	assertEquals( 234, fes->block_count() );
	assertEquals( 17, fes->blocks_required() );
	assertTrue( fes->good() );
}

TEST_CASE( "FountainStreamTest/testEncoder_ChangeBufferSize_Fails", "[unit]" )
{
	stringstream input;
	for (int i = 0; i < 100; ++i)
		input << "0123456789";

	fountain_encoder_stream::ptr fes = fountain_encoder_stream::create(input, 800);
	assertEquals( 0, fes->block_count() );
	assertEquals( 2, fes->blocks_required() );
	assertTrue( fes->good() );

	assertFalse( fes->restart_and_resize_buffer(1200) ); // larger than the buffer

	// stream left unchanged
	assertEquals( 0, fes->block_count() );
	assertEquals( 2, fes->blocks_required() );
	assertTrue( fes->good() );
}


TEST_CASE( "FountainStreamTest/testDecode", "[unit]" )
{
	stringstream input;
	for (int i = 0; i < 1000; ++i)
		input << "0123456789";

	fountain_encoder_stream::ptr fes = fountain_encoder_stream::create(input, 830);

	assertEquals( 0, fes->block_count() );
	assertEquals( 13, fes->blocks_required() );
	assertTrue( fes->good() );

	fountain_decoder_stream fds(input.str().size(), 830);

	std::array<char, 140> buff;
	for (int i = 0; i < 1000; ++i)
	{
		unsigned res = fes->readsome(buff.data(), buff.size());
		assertEquals( res, buff.size() );

		bool output = fds.write(buff.data(), buff.size());
		if (output)
		{
			auto reassembled = fds.recover();
			assertTrue( reassembled );
			assertEquals( input.str().size(), reassembled->size() );
			assertEquals( input.str(), string(reassembled->begin(), reassembled->end()) );
			break;
		}
		else if (i == 999)
			assertMsg(output, "couldn't decode :(");
	}

	// LT may need more blocks than blocks_required for recovery (probabilistic)
	assertEquals( 822, fds.block_size() );
	assertEquals( 10000, fds.data_size() );
	assertTrue( fds.good() );
	assertEquals( fes->blocks_required(), fds.blocks_required() );
	assertTrue( fds.progress() >= fds.blocks_required() );

	assertTrue( fes->block_count() >= fes->blocks_required() );
	assertEquals( 13, fes->blocks_required() );
	assertTrue( fes->good() );
}

TEST_CASE( "FountainStreamTest/testDecode_IgnoresDuplicateBlocksAndAcceptsLaterBlocks", "[unit]" )
{
	stringstream input;
	for (int i = 0; i < 1000; ++i)
		input << "0123456789";

	fountain_encoder_stream::ptr fes = fountain_encoder_stream::create(input, 830);
	assertTrue( fes->good() );

	fountain_decoder_stream fds(input.str().size(), 830);
	std::array<char, 830> firstBlock;
	std::array<char, 830> nextBlock;

	unsigned res = fes->readsome(firstBlock.data(), firstBlock.size());
	assertEquals( res, firstBlock.size() );
	assertFalse( fds.write(firstBlock.data(), firstBlock.size()) );
	assertEquals( 1, fds.progress() );

	assertFalse( fds.write(firstBlock.data(), firstBlock.size()) );
	assertEquals( 1, fds.progress() );

	bool decoded = false;
	for (int i = 0; i < 1000; ++i)
	{
		res = fes->readsome(nextBlock.data(), nextBlock.size());
		assertEquals( res, nextBlock.size() );
		decoded = fds.write(nextBlock.data(), nextBlock.size());
		if (decoded)
			break;
	}

	assertTrue( decoded );
	assertTrue( fds.progress() >= fds.blocks_required() );

	auto reassembled = fds.recover();
	assertTrue( reassembled );
	assertEquals( input.str(), string(reassembled->begin(), reassembled->end()) );
}

TEST_CASE( "FountainStreamTest/testDecode_BigPackets", "[unit]" )
{
	stringstream input;
	for (int i = 0; i < 1000; ++i)
		input << "0123456789";

	fountain_encoder_stream::ptr fes = fountain_encoder_stream::create(input, 830);

	assertEquals( 0, fes->block_count() );
	assertEquals( 13, fes->blocks_required() );
	assertTrue( fes->good() );

	fountain_decoder_stream fds(input.str().size(), 830);

	std::array<char, 830> buff; // one block per read/write
	for (int i = 0; i < 1000; ++i)
	{
		unsigned res = fes->readsome(buff.data(), buff.size());
		assertEquals( res, buff.size() );

		bool output = fds.write(buff.data(), buff.size());
		if (output)
		{
			auto reassembled = fds.recover();
			assertTrue( reassembled );
			assertEquals( input.str().size(), reassembled->size() );
			assertEquals( input.str(), string(reassembled->begin(), reassembled->end()) );
			break;
		}
		else if (i == 999)
			assertMsg(output, "couldn't decode :(");
	}

	// header 6→8: block_size = 830 - 8 = 822; LT may need more blocks
	assertEquals( 822, fds.block_size() );
	assertEquals( 10000, fds.data_size() );
	assertTrue( fds.good() );

	assertTrue( fes->block_count() >= fes->blocks_required() );
	assertEquals( 13, fes->blocks_required() );
	assertTrue( fes->good() );

}

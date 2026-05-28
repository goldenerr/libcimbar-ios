/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <cstdint>
#include <cstring>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <algorithm>

// ============ PRNG: splitmix64 — deterministic, fast ============
class LTRng
{
	uint64_t _state;
public:
	explicit LTRng(uint64_t seed) : _state(seed) {}

	uint64_t next()
	{
		uint64_t z = (_state += 0x9e3779b97f4a7c15ULL);
		z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
		z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
		return z ^ (z >> 31);
	}

	double next_double()
	{
		return (next() >> 11) * 0x1.0p-53;
	}

	uint64_t next_int(uint64_t max)
	{
		if (max <= 1)
			return 0;
		return next() % max;
	}
};

// ============ Robust Soliton distribution ============
//
// For large files (N > 10,000), the classic Ideal Soliton produces
// too few degree-1 blocks — P(d=1) = O(1/N). The peeling decoder
// stalls because the ripple of degree-1 blocks runs dry.
//
// Robust Soliton adds a spike τ(1) ≈ O(1/√N) of extra degree-1
// blocks, keeping the ripple alive for arbitrarily large N.
// References: Luby (2002) "LT Codes", §5.
//
inline unsigned lt_sample_degree(LTRng& rng, unsigned N)
{
	if (N <= 1)
		return 1;

	double u = rng.next_double();

	// Robust Soliton: minimum degree-1 fraction scales as 1/√N.
	// For N=100, this gives P(d=1)≈10%; for N=20,000, P(d=1)≈0.7%.
	// This is the τ(1) spike that sustains the peeling ripple.
	double p1_robust = 1.0 / sqrt((double)N);
	double p1 = (p1_robust > 2.0 / N) ? p1_robust : (2.0 / N);
	if (p1 > 0.5) p1 = 0.5;  // safety cap for tiny N (N<4)

	if (u < p1)
		return 1;

	// Remaining mass distributed as Ideal Soliton for d >= 2.
	// Ideal Soliton CDF for d≥2: F(d) = 1 - 1/d + 1/N ≈ 1 - 1/d.
	// Inverse sampling: d = floor(1/(1-u)).
	u = (u - p1) / (1.0 - p1);
	double t = 1.0 / (1.0 - u);
	unsigned d = (unsigned)(t);
	if (d < 2) d = 2;
	if (d > N) d = N;
	return d;
}

// ============ LT Encoder ============
class LTEncoder
{
	const uint8_t* _data;
	size_t _totalSize;
	size_t _packetSize;
	unsigned _N;

public:
	LTEncoder(const uint8_t* data, size_t total_size, size_t packet_size)
		: _data(data)
		, _totalSize(total_size)
		, _packetSize(packet_size)
	{
		_N = (unsigned)((total_size + packet_size - 1) / packet_size);
		if (_N < 1) _N = 1;
	}

	bool good() const
	{
		return _data != nullptr && _packetSize > 0 && _N > 0;
	}

	size_t packet_size() const
	{
		return _packetSize;
	}

	unsigned N() const { return _N; }

	// Encode block_num into output. Returns bytes written (always packet_size).
	size_t encode(unsigned block_num, uint8_t* output, size_t size)
	{
		if (size < _packetSize)
			return 0;

		LTRng rng((uint64_t)block_num);
		unsigned degree = lt_sample_degree(rng, _N);

		// Zero output buffer
		std::memset(output, 0, _packetSize);

		// Collect unique neighbor indices (degree is small, so set is fine)
		std::unordered_set<unsigned> neighbors;
		while (neighbors.size() < degree)
			neighbors.insert((unsigned)rng.next_int(_N));

		// XOR source blocks into output
		for (unsigned idx : neighbors)
		{
			size_t offset = idx * _packetSize;
			if (offset >= _totalSize)
				continue; // zero-padded — no effect

			size_t remaining = _totalSize - offset;
			size_t len = (remaining < _packetSize) ? remaining : _packetSize;
			for (size_t i = 0; i < len; ++i)
				output[i] ^= _data[offset + i];
		}

		return _packetSize;
	}
};

// ============ LT Decoder (incremental peeling) ============

struct ActiveBlock
{
	std::vector<unsigned> neighbors;      // remaining (shrinks during peeling)
	std::vector<uint8_t> data;            // current (XORed with recovered sources)
};

class LTDecoder
{
	unsigned _N;
	size_t _packetSize;
	size_t _totalSize;

	// Peeling state
	std::unordered_map<unsigned, ActiveBlock> _blocks;        // block_num -> active state
	std::vector<std::vector<unsigned>> _src_to_enc;            // source -> encoded block_nums
	std::vector<bool> _recovered;                              // source block recovered?
	std::vector<uint8_t> _output;                              // recovered data
	unsigned _recoveredCount;
	bool _needPeel;

public:
	LTDecoder(size_t total_size, size_t packet_size)
		: _packetSize(packet_size)
		, _totalSize(total_size)
		, _recoveredCount(0)
		, _needPeel(false)
	{
		_N = (unsigned)((total_size + packet_size - 1) / packet_size);
		if (_N < 1) _N = 1;
		_recovered.assign(_N, false);
		_src_to_enc.resize(_N);
		_output.resize(total_size);
	}

	bool good() const
	{
		return _packetSize > 0 && _N > 0;
	}

	unsigned progress() const
	{
		return _blocks.size();
	}

	size_t length() const
	{
		return _totalSize;
	}

	// Returns true if recovery just completed (all source blocks recovered)
	bool decode(unsigned block_num, uint8_t* data, size_t length)
	{
		// Already seen this block? Skip.
		if (_blocks.find(block_num) != _blocks.end())
			return false;

		if (length < _packetSize)
			return false;

		// Compute this block's original neighbors
		LTRng rng((uint64_t)block_num);
		unsigned degree = lt_sample_degree(rng, _N);

		std::unordered_set<unsigned> neighborSet;
		while (neighborSet.size() < degree)
			neighborSet.insert((unsigned)rng.next_int(_N));

		// Start with original data
		ActiveBlock ab;
		ab.data.assign(data, data + _packetSize);

		// XOR out already-recovered sources
		for (unsigned s : neighborSet)
		{
			if (_recovered[s])
			{
				size_t offset = s * _packetSize;
				size_t len = std::min(_packetSize, _totalSize - offset);
				for (size_t i = 0; i < len; ++i)
					ab.data[i] ^= _output[offset + i];
			}
			else
			{
				ab.neighbors.push_back(s);
				_src_to_enc[s].push_back(block_num);
			}
		}

		_blocks[block_num] = std::move(ab);

		// If degree became 1 (or was 1 to begin with), we can peel
		if (_blocks[block_num].neighbors.size() == 1)
			_needPeel = true;

		// Run peeling
		peel();

		return _recoveredCount == _N;
	}

	bool recover(uint8_t* data, unsigned size)
	{
		if (size < _totalSize)
			return false;

		peel(); // one last attempt

		if (_recoveredCount < _N)
			return false;

		std::copy(_output.begin(), _output.end(), data);
		return true;
	}

	unsigned recovered_count() const { return _recoveredCount; }

protected:
	void peel()
	{
		if (!_needPeel)
			return;

		// Build queue of degree-1 blocks
		std::queue<unsigned> q;
		for (auto& [bn, ab] : _blocks)
		{
			if (ab.neighbors.size() == 1)
				q.push(bn);
		}

		while (!q.empty())
		{
			unsigned bn = q.front(); q.pop();
			auto it = _blocks.find(bn);
			if (it == _blocks.end() || it->second.neighbors.empty())
				continue;

			unsigned s = it->second.neighbors[0];
			if (_recovered[s])
				continue;

			// Recover source block s
			size_t offset = s * _packetSize;
			size_t len = std::min(_packetSize, _totalSize - offset);
			std::copy(it->second.data.begin(), it->second.data.begin() + len, _output.data() + offset);
			_recovered[s] = true;
			_recoveredCount++;

			// XOR s out of all encoded blocks that reference it
			for (unsigned ebn : _src_to_enc[s])
			{
				auto eit = _blocks.find(ebn);
				if (eit == _blocks.end())
					continue;

				auto& eb = eit->second;
				auto nit = std::find(eb.neighbors.begin(), eb.neighbors.end(), s);
				if (nit == eb.neighbors.end())
					continue; // already removed

				eb.neighbors.erase(nit);

				// XOR the source data out
				for (size_t i = 0; i < len; ++i)
					eb.data[i] ^= _output[offset + i];

				if (eb.neighbors.size() == 1)
					q.push(ebn);
			}

			// Clear s from adjacency (no longer needed)
			_src_to_enc[s].clear();
		}

		_needPeel = false;
	}
};

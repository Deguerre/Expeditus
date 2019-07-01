#pragma once

#include <vector>
#include <algorithm>

class HashTable {
public:
	constexpr uint32_t hash(uint32_t v) {
		uint32_t h = (v >> 16);
		uint32_t l = (v & 0xFFFF);
		h ^= (l * 32771 + 20089) & 0xFFFF;
		l ^= (h * 32797 + 19927) & 0xFFFF;
		return (h << 16) | l;
	}

	constexpr uint32_t unhash(uint32_t v) {
		uint32_t h = (v >> 16);
		uint32_t l = (v & 0xFFFF);
		l ^= (h * 32797 + 19927) & 0xFFFF;
		h ^= (l * 32771 + 20089) & 0xFFFF;
		return (h << 16) | l;
	}

	uint32_t dib(uint32_t h, uint32_t pos) {
		const uint32_t modulus = data.size();
		return (pos - h) & (data.size() - 1);
	}

	HashTable()
	{
		const uint32_t s_default_size = 16;
		data.resize(s_default_size);
		for (auto& v : data) {
			v = s_tombstone;
		}
		mElements = 0;
	}

	const uint32_t s_tombstone = hash(0xFFFFFFFF);

	void insert(uint32_t v) {
		insert_hash(hash(v));
	}

	bool member(uint32_t v) {
		return search_hash(hash(v));
	}

	void erase(uint32_t v) {
		auto h = hash(v);
		auto loc = search_hash(h);
		if (!loc) {
			return;
		}
		*loc = s_tombstone;
		compress((uint32_t)(loc - &data[0]));
	}

private:
	void rehash() {
		std::vector<uint32_t> newdata((size_t)data.size() * 2, s_tombstone);
		std::swap(data, newdata);
		mElements = mTotalCost = mLongestProbe = 0;
		for (auto h : newdata) {
			if (h != s_tombstone) {
				insert_hash(h);
			}
		}
	}

	void insert_hash(uint32_t h) {
		if ((mElements * 14 + 15) >> 4 > data.size()) {
			rehash();
		}
		uint32_t mask = data.size() - 1;
		uint32_t probeposition = 0;
		++mTotalCost;
		while (h != s_tombstone) {
			++probeposition;
			auto location = (h + probeposition) & mask;
			++mTotalCost;
			auto recordposition = (location - data[location]) & mask;
			if (probeposition > recordposition) {
				std::swap(data[location], h);
				mLongestProbe = std::max<uint32_t>(mLongestProbe, probeposition);
				probeposition = recordposition;
			}
			++mTotalCost;
			mLongestProbe = std::max<uint32_t>(mLongestProbe, probeposition);
			++mElements;
		}
	}

	uint32_t* search_hash(uint32_t h) {
		uint32_t mask = data.size() - 1;
		uint32_t meanposition = mTotalCost / mElements;
		uint32_t downposition = meanposition;
		uint32_t upposition = downposition + 1;
		while (downposition >= 1 && upposition <= mLongestProbe) {
			auto downlocation = (h + downposition) & mask;
			if (data[downlocation] == h) {
				return &data[downlocation];
			}
			auto uplocation = (h + upposition) & mask;
			if (data[uplocation] == h) {
				return &data[uplocation];
			}
			--downposition;
			++upposition;
		}
		while (downposition >= 1) {
			auto downlocation = (h + downposition) & mask;
			if (data[downlocation] == h) {
				return &data[downlocation];
			}
			--downposition;
		}

		while (upposition <= mLongestProbe) {
			auto uplocation = (h + upposition) & mask;
			if (data[uplocation] == h) {
				return &data[uplocation];
			}
			++upposition;
		}

		return nullptr;
	}

	void compress(uint32_t pos) {
		const uint32_t mask = data.size() - 1;
		while (true) {
			uint32_t npos = (pos + 1) & mask;
			auto dnpos = data[npos];
			if (dnpos == s_tombstone) {
				break;
			}

			if ((dnpos & mask) == npos) {
				// dib is zero
				break;
			}

			data[pos] = dnpos;
			data[npos] = s_tombstone;
			pos = npos;
		}
	}


	uint32_t mElements = 0;
	uint32_t mTotalCost = 0;
	uint32_t mLongestProbe = 0;
	std::vector<uint32_t> data;
};

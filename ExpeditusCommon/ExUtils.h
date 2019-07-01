#pragma once

#include <algorithm>
#include <immintrin.h>
#include <ammintrin.h>


#define EXPEDITUS_CACHE_LINE_SIZE	128

namespace expeditus {

	inline unsigned rank(uint64_t v, unsigned i) {
		return __popcnt64(v & ((i ? (~(uint64_t)0) >> (64 - i) : (uint64_t)0)));
	}

	inline unsigned select(uint64_t v, unsigned i) {
		return _pdep_u64((uint64_t)1 << i, v);
	}

	inline unsigned ctz(uint64_t v) {
		return _tzcnt_u64(v);
	}

	inline unsigned lowest_set_bit(uint64_t v) {
		return _blsi_u64(v);
	}

	inline unsigned clear_lowest_set_bit(uint64_t v) {
		return _blsr_u64(v);
	}

	template<typename Container>
	void reserve_capacity(Container& container, size_t extra_space)
	{
		size_t capacity = container.capacity();
		size_t needed = container.size() + extra_space;
		if (capacity < needed) {
			container.reserve(std::max<size_t>(needed, (capacity * 5 + 3) >> 2));
		}
	}
}
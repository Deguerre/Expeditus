#pragma once

#include "ErsatzPriv.h"

#define II_SHIFT	5
#define II_MASK		(((uint64_t)1 << II_SHIFT) - 1)
#define II_BIT(p)	((uint64_t)1 << ((p) & II_MASK))
#define II_LINEAR_FALLBACK	(EXPEDITUS_CACHE_LINE_SIZE / 16)

struct InvertedIndex {
	std::vector<uint64_t> mask;
	std::vector<uint32_t> entries;

	void reset() {
		mask.clear();
		entries.clear();
	}

#if 0
	struct ConstIterator {
		const InvertedIndex& index;
		unsigned pos;

		uint32_t operator*() {
			return index.entries[pos];
		}

		void possibly_advance() {
			auto entries_size = index.entries.size();
			unsigned advance = 0;
			do {
				unsigned iibase = pos & ~II_MASK;
				unsigned ii = pos >> II_SHIFT;
				uint64_t mask = ~uint64_t(0) << (pos & II_MASK);
				advance = expeditus::ctz(index.mask[ii] & mask);
				pos = iibase + advance;
			} while (advance >= (1 << II_SHIFT) && pos < entries_size);
			if (pos > entries_size) {
				pos = entries_size;
			}
		}

		ConstIterator& operator++() {
			if (++pos < index.entries.size()) {
				possibly_advance();
			}
			return *this;
		}

		ConstIterator& operator++(int) {
			ConstIterator it(*this);
			++* this;
			return it;
		}

		bool operator!=(const ConstIterator& rhs) {
			return pos != rhs.pos;
		}

		ConstIterator(const InvertedIndex& index, size_t pos)
			: index(index), pos(pos)
		{
			if (pos < index.entries.size()) {
				possibly_advance();
			}
		}
	};

	ConstIterator begin() const
	{
		return ConstIterator(*this, 0);
	}

	ConstIterator end() const
	{
		return ConstIterator(*this, entries.size());
	}
#endif

	void mark(uint32_t e) {
		size_t pb = 0, pe = entries.size();
		while (pb + II_LINEAR_FALLBACK < pe) {
			auto pm = (pb + pe) >> 1;
			if (entries[pm] <= e) {
				pb = pm;
			}
			else {
				pe = pm;
			}
		}
		for (size_t i = pb; i < pe; ++i) {
			if (entries[i] == e) {
				mask[i >> II_SHIFT] &= ~II_BIT(i);
				return;
			}
		}
	}

	void unmark(uint32_t e) {
		size_t pb = 0, pe = entries.size();
		while (pb + II_LINEAR_FALLBACK < pe) {
			auto pm = (pb + pe) >> 1;
			if (entries[pm] <= e) {
				pb = pm;
			}
			else {
				pe = pm;
			}
		}
		for (size_t i = pb; i < pe; ++i) {
			if (entries[i] == e) {
				mask[i >> II_SHIFT] |= II_BIT(i);
				return;
			}
		}
	}

	void push_back(uint32_t e) {
		size_t pos = entries.size();
		if (!(pos & II_MASK)) {
			expeditus::reserve_capacity(mask, 1);
			entries.reserve(64);
			mask.push_back(0);
		}
		assert(!pos || entries.back() < e);
		entries.push_back(e);
		mask[pos >> II_SHIFT] |= II_BIT(pos);
	}
};



struct Variable {
	uint32_t
		value : 2,
		phase : 2,
		type : 1;
	int32_t antecedent_clause = -1;
	int dlevel = -1;
	InvertedIndex clauses;
	unsigned mentions[2] = { 0, 0 };

	void reset() {
		value = VAR_UNKNOWN;
		phase = VAR_UNKNOWN;
		antecedent_clause = -1;
		dlevel = -1;
		clauses.reset();
		mentions[0] = 0;
		mentions[1] = 0;
	}

	Variable(VariableType ty = VAR_PROBLEM)
	{
		type = ty;
		value = VAR_UNKNOWN;
		phase = VAR_UNKNOWN;
	}
};


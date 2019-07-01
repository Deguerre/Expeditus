#pragma once

#include "Ersatz.h"
#include "ExUtils.h"
#include <vector>
#include <cassert>
#include <cstdint>


#undef ERSATZ_DEBUG_DEDUCTION
#undef ERSATZ_TRACE_PROPAGATION
#define ERSATZ_TRACE_SETTING
#define ERSATZ_DEBUG_SOLVER
#undef ERSATZ_DEBUG_HEAP
#undef ERSATZ_USE_UNSATISFIED_CLAUSES
#define ERSATZ_COLLECT_SIZE_STATISTICS
#define ERSATZ_PRINT_RESULT

typedef uint32_t literal_t;

enum VariableState {
	VAR_TRUE = 0,
	VAR_FALSE,
	VAR_UNKNOWN
};

enum VariableType {
	VAR_PROBLEM,
	VAR_JOIN
};


enum ClauseType {
	CLAUSE_ORIGINAL,
	CLAUSE_CONFLICT
};


struct Literal {
	literal_t& lit;

	Literal(literal_t& lit)
		: lit(lit)
	{
	}

	unsigned index() const
	{
		return (unsigned)(lit >> 3);
	}

	VariableState direction() const
	{
		return (VariableState)(lit & 1);
	}

	void set(unsigned index, VariableState state)
	{
		lit = (index << 3) | (state == VAR_FALSE ? 1 : 0);
	}
};




struct HeapEntry {
	uint32_t num = ~uint32_t(0);
	uint32_t score = ~uint32_t(0);

	HeapEntry()
	{
	}

	HeapEntry(uint32_t num, uint32_t score)
		: num(num), score(score)
	{
	}

	bool operator<(const HeapEntry& rhs) const {
		return rhs.score < score;
	};

	bool operator<=(const HeapEntry& rhs) const {
		return rhs.score <= score;
	};
};

void breakpoint()
{
}

class Heap
{
	// This is an n-ary heap where n = 1<<HEAP_SHIFT.
	// Ideally, sizeof(HeapEntry) << HEAP_SHIFT should be about one cache line in size.
	// For 64-byte cache lines, that means 8 entries.

	static constexpr unsigned HEAP_SHIFT = 3;
	static constexpr unsigned HEAP_PLY = 1 << HEAP_SHIFT;
	static constexpr unsigned HEAP_START = HEAP_PLY - 1;
	static constexpr unsigned HEAP_OFFSET = HEAP_PLY - 2;
	constexpr unsigned heap_parent(unsigned i) { return (i >> HEAP_SHIFT) + HEAP_OFFSET; }
	constexpr unsigned heap_lchild(unsigned i) { return (i - HEAP_OFFSET) << HEAP_SHIFT; }
	constexpr unsigned heap_rchild(unsigned i) { return (i - HEAP_OFFSET) << HEAP_SHIFT + HEAP_PLY - 1; }

public:
	void increase_size(size_t n)
	{
		entries.reserve(n + HEAP_START);
		reverse.reserve(n);
		for (size_t i = 0; i < n; ++i) {
			reverse.push_back(0);
		}
#ifdef ERSATZ_DEBUG_HEAP
		verify_heap();
#endif
	}

	void clear()
	{
		entries.clear();
		entries.resize(HEAP_START);
		reverse.clear();
	}

	bool empty() const {
		return entries.size() <= HEAP_START;
	}

	bool member(uint32_t num) const {
		return reverse[num] != 0;
	}

	void insert(uint32_t num, uint32_t score) {
#ifdef ERSATZ_DEBUG_HEAP
		verify_heap();
#endif
		if (score > 0) {
			breakpoint();
		}
		reverse[num] = entries.size();
		entries.emplace_back(num, score);
		down_heap(reverse[num]);
#ifdef ERSATZ_DEBUG_HEAP
		verify_heap();
#endif
	}

	void adjust(uint32_t pos, uint32_t new_score)
	{
#ifdef ERSATZ_DEBUG_HEAP
		verify_heap();
#endif
		auto index = reverse[pos];
		uint32_t old_score = entries[index].score;
		if (old_score == new_score) {
			return;
		}
		adjust_internal(index, new_score);
#ifdef ERSATZ_DEBUG_HEAP
		verify_heap();
#endif
	}

	void remove(uint32_t num) {
#ifdef ERSATZ_DEBUG_HEAP
		verify_heap();
#endif
		auto index = reverse[num];
		auto old_score = entries[index].score;
		if ((size_t)index + 1 != entries.size()) {
			auto last_place_index = entries.size() - 1;
			auto last_place_num = entries[last_place_index].num;
			auto last_place_score = entries[last_place_index].score;
			entries[index].num = last_place_num;
			reverse[last_place_num] = index;
			adjust_internal(index, last_place_score);
		}
		entries.pop_back();
		reverse[num] = 0;
#ifdef ERSATZ_DEBUG_HEAP
		verify_heap();
#endif
	}

	uint32_t head() const {
		return entries[HEAP_START].num;
	}

private:
	std::vector<HeapEntry> entries;
	std::vector<uint32_t> reverse;

	void move(uint32_t from, uint32_t to)
	{
		entries[to] = entries[from];
		reverse[entries[to].num] = to;
	}

	void down_heap(uint32_t index)
	{
		auto entry = entries[index];
		auto parent = heap_parent(index);
		while (index > HEAP_START + 1 && entry < entries[parent]) {
			move(parent, index);
			index = parent;
			parent = heap_parent(index);
		}
		entries[index] = entry;
		reverse[entries[index].num] = index;
	}

	void up_heap(uint32_t index)
	{
		unsigned n = entries.size();
		auto entry = entries[index];
		unsigned left_child = heap_lchild(index);
		unsigned right_child = std::min<unsigned>(heap_rchild(index), n - 1);
		while (left_child < n && right_child < n) {
			unsigned child_index = left_child;
			for (unsigned c = left_child + 1; c <= right_child; ++c) {
				if (entries[c] < entries[child_index]) {
					child_index = c;
				}
			}
			if (entry <= entries[child_index]) {
				break;
			}
			move(child_index, index);
			index = child_index;
			left_child = heap_lchild(index);
			right_child = std::min<unsigned>(heap_rchild(index), n - 1);
		}
		entries[index] = entry;
		reverse[entries[index].num] = index;
	}

	void adjust_internal(uint32_t index, uint32_t new_score)
	{
		auto& entry = entries[index];
		HeapEntry new_entry(entry.num, new_score);

		if (new_entry < entry) {
			entry.score = new_score;
			down_heap(index);
		}
		else if (entry < new_entry) {
			entry.score = new_score;
			up_heap(index);
		}
	}


#ifdef ERSATZ_DEBUG_HEAP
	void verify_heap()
	{
		for (unsigned i = HEAP_PLY; i < entries.size(); ++i) {
			auto parent = heap_parent(i);
			assert(entries[parent] <= entries[i]);
		}
		for (unsigned i = 0; i < reverse.size(); ++i) {
			if (reverse[i] == 0) {
				continue;
			}
			assert(reverse[i] < entries.size());
			assert(entries[reverse[i]].num == i);
		}
	}
#endif
};




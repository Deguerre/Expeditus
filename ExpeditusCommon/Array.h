#pragma once

#include "Memory.h"

struct Array
{
	uint32_t* begin;
	uint32_t* end;
	uint32_t* data;

	uint32_t& operator[](uint32_t i) {
		return begin[i];
	}

	const uint32_t& operator[](uint32_t i) const {
		return begin[i];
	}

};


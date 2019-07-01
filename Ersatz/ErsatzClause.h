#pragma once

#include "ErsatzPriv.h"

#define ERSATZ_MAX_CLAUSE_SIZE	64

struct Clause {
	uint32_t
		literal_count : 8,
		type : 1,
		join_subsequent : 1,
		mark : 1;
	literal_t literals[ERSATZ_MAX_CLAUSE_SIZE];

	void reset() {
		mark = 0;
	}

	Clause(ClauseType ct, unsigned litcount)
	{
		literal_count = litcount;
		type = ct;
		mark = 0;
		join_subsequent = 0;
	}
};

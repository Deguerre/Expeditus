#pragma once

#ifdef ERSATZ_DLL
#define ERSATZ_API   __declspec(dllexport)
#else
#define ERSATZ_API   __declspec(dllimport)
#endif

#include <memory>

class ERSATZ_API Ersatz {
public:
	enum outcome {
		OUTCOME_UNDETERMINED = 0,
		OUTCOME_UNSATISFIABLE,
		OUTCOME_SATISFIABLE
	};

	Ersatz();
	~Ersatz();

	void reserve(int num_vars, int num_clauses);
	void set_num_variables(int num_vars);
	int add_variable();
	void add_clause(int* begin, int* end);

	void solve();

	void reset_solution();
	int num_variables() const;
	bool query_assignment(int var) const;

private:
	struct Impl;
	std::unique_ptr<Impl> mPimpl;
};


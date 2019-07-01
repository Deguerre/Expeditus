// Ersatz.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "ErsatzPriv.h"
#include "ErsatzVariable.h"
#include "ErsatzClause.h"
#include <deque>
#include <iostream>
#include <unordered_set>
#ifdef ERSATZ_COLLECT_SIZE_STATISTICS
#include <map>
#endif


struct Implication {
	literal_t literal;
	int antecedent_clause;
};


struct ConflictingClause {
	uint32_t clause;
	uint32_t last_var;

	ConflictingClause(uint32_t clause, uint32_t last_var)
		: clause(clause), last_var(last_var)
	{
	}
};


struct Ersatz::Impl {
	struct StackEntry {
		uint32_t asgn_ptr;
		uint32_t clause_ptr;
		StackEntry(uint32_t asgn_ptr, uint32_t clause_ptr)
			: asgn_ptr(asgn_ptr), clause_ptr(clause_ptr)
		{
		}
	};

	struct Statistics {
		unsigned num_decisions;
		unsigned num_backtracks;
		unsigned num_conflict_clauses;
		unsigned num_restarts;
		unsigned num_split_clauses;
#ifdef ERSATZ_COLLECT_SIZE_STATISTICS
		std::map<uint32_t, uint32_t> conflict_clause_sizes;
#endif

		void reset()
		{
			num_decisions = 0;
			num_backtracks = 0;
			num_conflict_clauses = 0;
			num_restarts = 0;
			num_split_clauses = 0;
		}

		Statistics()
		{
			reset();
		}
	};

	Statistics stats;
	Ersatz::outcome outcome;
	bool currently_solving = 0;

	std::vector<Variable> vars;
	std::vector<Clause> clauses;

	static const uint32_t vsids_score_max = 1u << 30;
	static const uint32_t vsids_decay_default = 2;
	static const uint32_t vsids_decay_numerator = 230;
	static const int32_t vsids_threshold = 32;
	uint32_t vsids_decay;

	struct vsids_activity {
		uint32_t scores[2] = { 0,0 };
		uint32_t score() const { return scores[0] + scores[1]; }
		VariableState direction(VariableState preference) const {
			int difference = scores[VAR_TRUE] - scores[VAR_FALSE];
			if (difference > vsids_threshold) {
				return VAR_TRUE;
			}
			else if (difference < -vsids_threshold) {
				return VAR_FALSE;
			}
			else {
				return preference;
			}
		}
	};
	std::vector<vsids_activity> vsids_score;

	void vsids_init() {
		vsids_score.clear();
		vsids_score.reserve(vars.size());
		for (auto& v : vars) {
			vsids_score.emplace_back();
		}
		vsids_decay = vsids_decay_default;
	}

	void vsids_bump(uint32_t var, uint32_t adjust, int value) {
		if ((vsids_score[var].scores[value] += adjust) > vsids_score_max) {
			for (auto& score : vsids_score) {
				score.scores[0] >>= 2;
				score.scores[1] >>= 2;
			}
		}
	}

	void vsids_conflict() {
		if (--vsids_decay > 0) {
			return;
		}
		for (auto& score : vsids_score) {
			score.scores[0] = (uint32_t)(((uint64_t)score.scores[0] * vsids_decay_numerator) >> 8);
			score.scores[1] = (uint32_t)(((uint64_t)score.scores[1] * vsids_decay_numerator) >> 8);
		}
		vsids_decay = vsids_decay_default;
	}

	std::deque<Implication> implications;
	std::deque<ConflictingClause> conflicting_clauses;

	void set_num_variables(unsigned nv)
	{
		vars.resize(nv);
	}

	int add_variable(VariableType ty)
	{
		int v = vars.size();
		expeditus::reserve_capacity(vars, 1);
		vars.emplace_back(ty);
		if (currently_solving) {
			vsids_score.emplace_back();
			unassigned_vars.increase_size(1);
			unassigned_vars.insert(v, 0);
		}
		return v;
	}

	void reserve_vars(unsigned nv)
	{
		vars.reserve(nv);
	}

	void reserve_clauses(unsigned nc)
	{
		clauses.reserve(nc);
	}

	uint32_t add_clause_(const literal_t* lits, size_t num_lits, ClauseType type)
	{
		unsigned unknown_count = 0;
		unsigned clause_num = clauses.size();
		for (unsigned i = 0; i < num_lits; ++i) {
			auto l = lits[i];
			Literal lit(l);
			auto& var = vars[lit.index()];
			var.clauses.push_back(clause_num);
			++var.mentions[lit.direction()];
			if (type == CLAUSE_CONFLICT) {
				vsids_bump(lit.index(), 10000, lit.direction());
			}
			if (var.value == VAR_UNKNOWN) {
				++unknown_count;
			}
		}

		Clause clause(type, num_lits);
		for (unsigned i = 0; i < num_lits; ++i) {
			clause.literals[i] = lits[i];
		}
		clauses.emplace_back(std::move(clause));
		if (type == CLAUSE_CONFLICT) {
			++stats.num_conflict_clauses;
#ifdef ERSATZ_USE_UNSATISFIED_CLAUSES
			unsatisfied_clauses.increase_size(1);
			unsatisfied_clauses.insert(clause_num, unknown_count);
#endif
		}
		return clause_num;
	}

	uint32_t add_clause(const std::vector<literal_t>& lits, ClauseType type)
	{
		auto num_lits = lits.size();
#ifdef ERSATZ_COLLECT_SIZE_STATISTICS
		if (type == CLAUSE_CONFLICT) {
			++stats.conflict_clause_sizes[num_lits];
		}
#endif
		if (num_lits > ERSATZ_MAX_CLAUSE_SIZE) {
			int new_vars = num_lits + 2 - 2*ERSATZ_MAX_CLAUSE_SIZE;
			if (new_vars < 1) new_vars = 1;
			expeditus::reserve_capacity(vars, new_vars);
			std::vector<literal_t> sublits;
			sublits.reserve(ERSATZ_MAX_CLAUSE_SIZE);
			for (unsigned i = 0; i + 1 < ERSATZ_MAX_CLAUSE_SIZE; ++i) {
				sublits.push_back(lits[i]);
			}

			literal_t join_lit;
			Literal join_lit_l(join_lit);
			join_lit_l.set(add_variable(VAR_JOIN), VAR_TRUE);
			sublits.push_back(join_lit);
			auto first_clause = add_clause_(&sublits[0], sublits.size(), type);

			unsigned begin = ERSATZ_MAX_CLAUSE_SIZE - 1;
			unsigned end = num_lits;

			while (ERSATZ_MAX_CLAUSE_SIZE + begin < end + 1) {
				sublits.clear();
				join_lit_l.set(join_lit_l.index(), VAR_FALSE);
				sublits.push_back(join_lit);
				for (unsigned i = 0; i + 2 < ERSATZ_MAX_CLAUSE_SIZE; ++i) {
					sublits.push_back(lits[begin + i]);
				}
				join_lit_l.set(add_variable(VAR_JOIN), VAR_TRUE);
				sublits.push_back(join_lit);
				++stats.num_split_clauses;
				clauses[add_clause_(&sublits[0], sublits.size(), type)].join_subsequent = 1;
				begin += ERSATZ_MAX_CLAUSE_SIZE - 2;
			}
			if (begin < end) {
				sublits.clear();
				join_lit_l.set(join_lit_l.index(), VAR_FALSE);
				sublits.push_back(join_lit);
				for (unsigned i = begin; i < end; ++i) {
					sublits.push_back(lits[i]);
				}
				clauses[add_clause_(&sublits[0], sublits.size(), type)].join_subsequent = 1;
				++stats.num_split_clauses;
			}
			return first_clause;
		}
		else {
			return add_clause_(&lits[0], lits.size(), type);
		}
	}

	void add_clause(int* begin, int* end)
	{
		if (begin > end) {
			throw 0;
		}
		size_t s = end - begin;
		std::vector<literal_t> literals;
		literals.reserve(s);
		for (unsigned i = 0; i < s; ++i) {
			literal_t l;
			Literal lit(l);
			unsigned var = begin[i] >> 1;
			lit.set(var, (begin[i] & 1) ? VAR_TRUE : VAR_FALSE);
			literals.push_back(l);
		}
		add_clause(literals, CLAUSE_ORIGINAL);
	}

	void enqueue_implication(literal_t lit, int antecedent_clause)
	{
		Implication impl;
		impl.literal = lit;
		impl.antecedent_clause = antecedent_clause;
		implications.push_back(impl);
	}

	void init_solve();
	void solve();
	void solver();

	void reset_solution();

	// Detect any trivial deductions, return false if unsatisfiable.
	bool preprocess();

	// Decide on the next assignment, return false if there are no options.
	bool decide();

	// Make deductions, return true if this results in a conflict.
	bool deduce();

	// Analyse conflicts, return true if this would backtrack to the top level.
	bool analyse_conflicts();

	int conflict_standard();
	int conflict_1UIP();

	int dlevel;
	std::vector<literal_t> assignment_stack;
	std::vector<uint32_t> clause_stack;
	std::deque<StackEntry> stack;
	void backtrack(int level);

	static const unsigned restart_shift = 5;
	int64_t restart_u;
	uint64_t restart_v;
	unsigned restart_count;

	void restart_init()
	{
		restart_u = 1;
		restart_v = 1;
		restart_count = 1 << restart_shift;
	}

	bool possibly_restart()
	{
		if (--restart_count > 0) {
			return false;
		}
		++stats.num_restarts;
		if (expeditus::lowest_set_bit(restart_u) == restart_v) {
			++restart_u;
			restart_v = 1;
		}
		else {
			restart_v <<= 1;
		}
		restart_count = restart_v << restart_shift;
		return true;
	}

#ifdef ERSATZ_USE_UNSATISFIED_CLAUSES
	Heap unsatisfied_clauses;
#endif
	Heap unassigned_vars;

	void trail_assignment(literal_t lit) {
		expeditus::reserve_capacity(assignment_stack, 1);
		assignment_stack.push_back(lit);
	}

	void trail_clause(int32_t clause) {
		expeditus::reserve_capacity(clause_stack, 1);
		clause_stack.push_back(clause);
	}

	void set_variable(uint32_t var, VariableState val, int antecedent_clause);
	void unset_variable(uint32_t var);

	void satisfy_clause(uint32_t clause);
	void unsatisfy_clause(uint32_t clause);
};

void Ersatz::Impl::satisfy_clause(uint32_t clause)
{
	auto& c = clauses[clause];
	if (c.mark) {
		return;
	}

	for (unsigned li = 0; li < c.literal_count; ++li) {
		auto& l = c.literals[li];
		Literal lit(l);
		auto& var = vars[lit.index()];
		--var.mentions[lit.direction()];
		var.clauses.mark(clause);
	}
	c.mark = true;
	trail_clause(clause);
#ifdef ERSATZ_USE_UNSATISFIED_CLAUSES
	if (c.type == CLAUSE_CONFLICT) {
		unsatisfied_clauses.remove(clause);
	}
#endif
}


void Ersatz::Impl::unsatisfy_clause(uint32_t clause)
{
	auto& c = clauses[clause];
	if (!c.mark) {
		return;
	}

	c.mark = false;
	unsigned unsat_lits = 0;
	for (unsigned li = 0; li < c.literal_count; ++li) {
		auto& l = c.literals[li];
		Literal lit(l);
		auto& v = vars[lit.index()];
		++v.mentions[lit.direction()];
		v.clauses.unmark(clause);
		if (v.value == VAR_UNKNOWN) {
			++unsat_lits;
		}
	}
#ifdef ERSATZ_USE_UNSATISFIED_CLAUSES
	if (c.type == CLAUSE_CONFLICT) {
		unsatisfied_clauses.insert(clause, unsat_lits);
		}
#endif
	}

void
Ersatz::Impl::unset_variable(uint32_t var)
{
	auto& v = vars[var];
	if (v.dlevel < dlevel) v.phase = v.value;
	v.value = VAR_UNKNOWN;
	v.dlevel = -1;
	v.antecedent_clause = -1;
	unassigned_vars.insert(var, vsids_score[var].score());
}


void
Ersatz::Impl::set_variable(uint32_t var, VariableState val, int antecedent_clause)
{
	auto& v = vars[var];
	v.value = val;
	v.dlevel = dlevel;
	v.antecedent_clause = antecedent_clause;
	{
		literal_t l;
		Literal lit(l);
		lit.set(var, (VariableState)v.value);
		trail_assignment(l);
#ifdef ERSATZ_TRACE_SETTING
		std::cerr << "Setting variable " << (val == VAR_TRUE ? '+' : '-') << lit.index() << '\n';
#endif
	}

	unassigned_vars.remove(var);
	auto mask_size = v.clauses.mask.size();
	for (unsigned mi = 0; mi < mask_size; ++mi) {
		auto mask = v.clauses.mask[mi];
		while (mask) {
			auto bit = expeditus::lowest_set_bit(mask);
			auto i = v.clauses.entries[(mi << II_SHIFT) | expeditus::ctz(bit)];
			mask = expeditus::clear_lowest_set_bit(mask);
			auto & c = clauses[i];
#if 0
			if (c.mark) {
				continue;
			}
#endif
#ifdef ERSATZ_DEBUG_DEDUCTION
			std::cerr << "  Processing clause " << i << ':';
#endif

			unsigned satisfied = 0;
			unsigned unassigned = 0;
			literal_t unassigned_literal = 0;
			for (unsigned li = 0; li < c.literal_count; ++li) {
				auto& l = c.literals[li];
				Literal lit(l);
				auto& variable = vars[lit.index()];
#ifdef ERSATZ_DEBUG_DEDUCTION
				std::cerr << ' ' << (lit.direction() == VAR_TRUE ? '+' : '-') << lit.index();
#endif

				if (variable.value == VAR_UNKNOWN) {
					++unassigned;
					unassigned_literal = l;
				}
				else if (variable.value == lit.direction()) {
					++satisfied;
				}
			}
#ifdef ERSATZ_DEBUG_DEDUCTION
			std::cerr << "\n  satisfied = " << satisfied << ", unassigned = " << unassigned << '\n';
#endif

			if (satisfied > 0) {
				// A satisfied clause.
				satisfy_clause(i);
#ifdef ERSATZ_DEBUG_DEDUCTION
				std::cerr << "  This clause is now satisfied\n";
#endif
			}
			else if (unassigned == 1) {
				// One unassigned literal, so this is an implication.
				enqueue_implication(unassigned_literal, i);
				Literal lit(unassigned_literal);
#ifdef ERSATZ_DEBUG_DEDUCTION
				std::cerr << "  This implies " << (lit.direction() == VAR_TRUE ? '+' : '-') << lit.index() << '\n';
#endif
			}
			else if (!unassigned) {
				// No unassigned or satisfied literals; this is a conflict.
				conflicting_clauses.emplace_back(i, var);
#ifdef ERSATZ_DEBUG_DEDUCTION
				std::cerr << "  This is a conflict\n";
#endif
			}
#ifdef ERSATZ_USE_UNSATISFIED_CLAUSES
			else if (c.type == CLAUSE_CONFLICT) {
				// Conflict clause's score needs to be adjusted
				unsatisfied_clauses.adjust(i, unassigned);
			}
#endif
		}
	}
}


void
Ersatz::Impl::backtrack(int level)
{
	assert(level <= dlevel);

	if (stack.empty()) {
		return;
	}

	unsigned target_asgn_ptr = stack[level].asgn_ptr;
	unsigned target_clause_ptr = stack[level].clause_ptr;

	while (assignment_stack.size() > target_asgn_ptr) {
		auto l = assignment_stack.back();
		assignment_stack.pop_back();
		Literal lit(l);
		unset_variable(lit.index());
	}

	while (clause_stack.size() > target_clause_ptr) {
		auto c = clause_stack.back();
		clause_stack.pop_back();
		unsatisfy_clause(c);
	}

	while (stack.size() > level) {
		stack.pop_back();
	}

	dlevel = level;
	++stats.num_backtracks;
}


// Detect any trivial deductions, return false if unsatisfiable.
bool
Ersatz::Impl::preprocess()
{
	// Find unused variables.

#ifdef ERSATZ_DEBUG_SOLVER
	std::deque<literal_t> interesting_vars;
#endif
	for (unsigned v = 0; v < vars.size(); ++v) {
		auto& var = vars[v];
		if (var.value != VAR_UNKNOWN) {
			continue;
		}
		if (var.mentions[0] == 0 && var.mentions[1] == 0) {
#ifdef ERSATZ_DEBUG_SOLVER
			interesting_vars.push_back(v);
#endif
			literal_t l;
			Literal lit(l);
			lit.set(v, VAR_TRUE);
			enqueue_implication(l, -1);
			bool unsat = deduce();
			assert(!unsat);
		}
	}

#ifdef ERSATZ_DEBUG_SOLVER
	if (!interesting_vars.empty()) {
		std::cerr << "Unused variables:";
		for (auto v : interesting_vars) {
			std::cerr << ' ' << v;
		}
		std::cerr << '\n';
		interesting_vars.clear();
	}
#endif

	// Find uniphase variables
	for (unsigned v = 0; v < vars.size(); ++v) {
		auto& var = vars[v];
		if (var.value != VAR_UNKNOWN) {
			continue;
		}

		literal_t l;
		Literal lit(l);
		if (var.mentions[0] == 0) {
			lit.set(v, VAR_FALSE);
			enqueue_implication(l, -1);
#ifdef ERSATZ_DEBUG_SOLVER
			interesting_vars.push_back(l);
#endif
		}
		else if (var.mentions[1] == 0) {
			lit.set(v, VAR_TRUE);
			enqueue_implication(l, -1);
#ifdef ERSATZ_DEBUG_SOLVER
			interesting_vars.push_back(l);
#endif
		}
	}

#ifdef ERSATZ_DEBUG_SOLVER
	if (!interesting_vars.empty()) {
		std::cerr << "Uniphase vars:";
		for (auto v : interesting_vars) {
			Literal lit(v);
			std::cerr << ' ' << (lit.direction() == VAR_TRUE ? '+' : '-') << lit.index();
		}
		std::cerr << '\n';
		interesting_vars.clear();
	}
#endif

	// Find unit clauses
	for (unsigned i = 0; i < clauses.size(); ++i) {
		auto& cl = clauses[i];
		if (cl.mark) {
			continue;
		}
		if (cl.literal_count == 1) {
			Literal lit(cl.literals[0]);
#ifdef ERSATZ_DEBUG_SOLVER
			std::cerr << "    Unit clause: " << (lit.direction() == VAR_TRUE ? '+' : '-') << lit.index() << '\n';
#endif
			if (vars[lit.index()].value == VAR_UNKNOWN) {
				enqueue_implication(lit.lit, i);
			}
		}
	}

	if (implications.size() > 0) {
		if (deduce()) {
			return false;
		}
	}

	for (auto& v : vars) {
		v.phase = v.mentions[VAR_TRUE] > v.mentions[VAR_FALSE] ? VAR_TRUE : VAR_FALSE;
	}

	return true;
}


void
Ersatz::Impl::solver()
{
	while (outcome == OUTCOME_UNDETERMINED) {
		if (decide()) {
			while (deduce()) {
				if (analyse_conflicts()) {
#ifdef ERSATZ_PRINT_RESULT
					std::cerr << "Unsatisfiable!\n";
#endif
					outcome = OUTCOME_UNSATISFIABLE;
					return;
				}
			}
		}
		else {
#ifdef ERSATZ_PRINT_RESULT
			std::cerr << "Satisfiable!\n";
			std::cerr << "Positive vars:";
			for (unsigned i = 0; i < vars.size(); ++i) {
				auto& v = vars[i];
				if (v.value == VAR_TRUE) {
					std::cerr << ' ' << i;
				}
			}
			std::cerr << '\n';
#endif

			outcome = OUTCOME_SATISFIABLE;
		}
	}
}


// Decide on the next assignment, return false if there are no options.
bool
Ersatz::Impl::decide()
{
	if (unassigned_vars.empty()) {
#ifdef ERSATZ_DEBUG_SOLVER
		std::cerr << "  No choices\n";
#endif
		return false;
	}

#ifdef ERSATZ_DEBUG_SOLVER
	std::cerr << "Picking a variable to decide on\n";
#endif

	literal_t l;
	Literal lit(l);

	bool found = false;
	int clauses_to_try = 256;
	for (int c = clauses.size() - 1; c >= 0 ; --c) {
		if (--clauses_to_try < 0) {
			break;
		}
		auto& clause = clauses[c];
		if (clause.type != CLAUSE_CONFLICT || clause.mark) {
			continue;
		}
#ifdef ERSATZ_DEBUG_SOLVER
		std::cerr << "Examining clause " << c << "\n";
#endif
		int best_score = -1;
		for (unsigned li = 0; li < clause.literal_count; ++li) {
			auto clause_literal = clause.literals[li];
			Literal clause_lit(clause_literal);
			if (vars[clause_lit.index()].value != VAR_UNKNOWN) {
				continue;
			}
			auto& vsids = vsids_score[clause_lit.index()];
			int score = vsids.score();
			if (score > best_score) {
				clause_lit.set(clause_lit.index(), vsids.direction(clause_lit.direction()));
				best_score = score;
				l = clause_literal;
				found = true;
			}
		}
#ifdef ERSATZ_DEBUG_SOLVER
		std::cerr << "    Choosing " << (lit.direction() == VAR_TRUE ? '+' : '-') << lit.index() << '\n';
#endif
		break;
	}

	if (!found) {
		auto v = unassigned_vars.head();
		auto& var = vars[v];
		assert(var.value == VAR_UNKNOWN);
#ifdef ERSATZ_DEBUG_SOLVER
		std::cerr << "    Found unset var " << v << '\n';
#endif
		auto& vsids = vsids_score[v];
		lit.set(v, vsids.direction((VariableState)var.phase));
#ifdef ERSATZ_DEBUG_SOLVER
		std::cerr << "    Choosing " << (lit.direction() == VAR_TRUE ? '+' : '-') << lit.index() << '\n';
#endif
	}

	++dlevel;
	stack.emplace_back(StackEntry((uint32_t)assignment_stack.size(), (uint32_t)clause_stack.size()));
	enqueue_implication(l, -1);
	++stats.num_decisions;
	return true;
}

// Make deductions, return true if this results in a conflict.
bool
Ersatz::Impl::deduce()
{
	while (!implications.empty() && conflicting_clauses.empty()) {
		Implication impl = implications.front();
		implications.pop_front();
		Literal lit(impl.literal);
		auto& variable = vars[lit.index()];
#ifdef ERSATZ_TRACE_PROPAGATION
		std::cerr << "Propagating " << (lit.direction() == VAR_TRUE ? '+' : '-') << lit.index() << '\n';
#endif

		if (variable.value == VAR_UNKNOWN) {
			// This is an assignment.
			set_variable(lit.index(), lit.direction(), impl.antecedent_clause);
		}
		else if (variable.value != lit.direction()) {
			// This is a conflict.
			conflicting_clauses.emplace_back(impl.antecedent_clause, lit.index());
		}
		else {
			// Variable has been assigned before.
		}
	}
	if (!implications.empty()) {
		implications.clear();
	}
	return !conflicting_clauses.empty();
}

// Analyse conflicts, return true if this would backtrack to the top level.
bool 
Ersatz::Impl::analyse_conflicts()
{
	if (dlevel == 0) {
		conflicting_clauses.clear();
		backtrack(0);
		return true;
	}

	auto new_dlevel = conflict_1UIP();

	if (possibly_restart()) {
		backtrack(0);
	}
	else {
		if (new_dlevel < 0) new_dlevel = 0;
#ifdef ERSATZ_DEBUG_SOLVER
		std::cerr << "Backtracking from " << dlevel << " to " << new_dlevel << '\n';
#endif
		backtrack(new_dlevel);
	}
	conflicting_clauses.clear();
	vsids_conflict();
	return false;
}


int
Ersatz::Impl::conflict_1UIP()
{
	auto c = conflicting_clauses.front();
	auto& clause = clauses[c.clause];
	std::unordered_set<literal_t> conflict_clause_set;
	std::unordered_set<uint32_t> seen_vars;
	int new_dlevel = -1;

	unsigned current_level_literals = 0;
#ifdef ERSATZ_DEBUG_SOLVER
	std::cerr << "Generating conflict clause " << c.clause << ':';
#endif
	for (unsigned li = 0; li < clause.literal_count; ++li) {
		auto& l = clause.literals[li];
		Literal lit(l);
		auto v = lit.index();
		auto& var = vars[v];
		if (var.dlevel == 0) {
			continue;
		}
		if (var.dlevel < dlevel) {
			conflict_clause_set.insert(l);
			new_dlevel = std::max<int>(new_dlevel, var.dlevel);
		}
		else {
			++current_level_literals;
		}
		seen_vars.insert(lit.index());
	}

	for (auto it = assignment_stack.rbegin(); it != assignment_stack.rend(); ++it) {
		Literal lit(*it);
		auto var = lit.index();
		auto& v = vars[var];
		if (!seen_vars.count(var)) {
			continue;
		}
		if (current_level_literals-- == 1) {
			literal_t neglit = *it;
			Literal negl(neglit);
			negl.set(var, v.value == VAR_TRUE ? VAR_FALSE : VAR_TRUE);
			conflict_clause_set.insert(neglit);
			break;
		}
		if (v.antecedent_clause != -1) {
			auto& ante_clause = clauses[v.antecedent_clause];
			for (unsigned anteli = 0; anteli < ante_clause.literal_count; ++anteli) {
				auto& antel = ante_clause.literals[anteli];
				Literal antelit(antel);
				auto antevar = antelit.index();
				auto& antev = vars[antevar];
				if (seen_vars.count(antevar) || antev.dlevel == 0) {
					continue;
				}
				if (antev.dlevel < dlevel) {
					conflict_clause_set.insert(antel);
					new_dlevel = std::max<int>(new_dlevel, antev.dlevel);
				}
				else {
					++current_level_literals;
				}
				seen_vars.insert(antevar);
			}
		}


	}

	std::vector<literal_t> conflict_clause;
	conflict_clause.reserve(conflict_clause_set.size());
	for (auto l : conflict_clause_set) {
		conflict_clause.push_back(l);
		Literal lit(l);
#ifdef ERSATZ_DEBUG_SOLVER
		std::cerr << ' ' << (lit.direction() == VAR_TRUE ? '+' : '-') << lit.index();
#endif
	}
#ifdef ERSATZ_DEBUG_SOLVER
	std::cerr << '\n';
#endif

	if (conflict_clause.size() < 2) {
		enqueue_implication(conflict_clause.front(), -1);
		new_dlevel = 1;
	}
	add_clause(std::move(conflict_clause), CLAUSE_CONFLICT);
	return new_dlevel < dlevel ? new_dlevel - 1 : dlevel - 1;
}



int
Ersatz::Impl::conflict_standard()
{
	auto c = conflicting_clauses.front();
	auto& clause = clauses[c.clause];
	std::unordered_set<literal_t> conflict_clause_set;
	std::unordered_set<uint32_t> seen_vars;
	std::deque<uint32_t> traverse_vars;
	int new_dlevel = -1;

	std::cerr << "Generating conflict clause " << c.clause << ':';
	for (unsigned li = 0; li < clause.literal_count; ++li) {
		auto& l = clause.literals[li];
		Literal lit(l);
		auto v = lit.index();
		auto& var = vars[v];
		traverse_vars.push_back(lit.index());
		seen_vars.insert(lit.index());
	}

	while (!traverse_vars.empty()) {
		auto v = traverse_vars.front();
		traverse_vars.pop_front();
		auto& var = vars[v];
		if (var.dlevel == 0) {
			// Ignore backbone literals
			continue;
		}

		if (var.dlevel < dlevel || var.antecedent_clause < 0) {
			literal_t lit;
			Literal l(lit);
			l.set(v, var.value == VAR_TRUE ? VAR_FALSE : VAR_TRUE);
			conflict_clause_set.insert(lit);
			if (var.dlevel < dlevel && var.dlevel > new_dlevel) new_dlevel = var.dlevel;
		}
		else {
			auto& ante_clause = clauses[var.antecedent_clause];
			for (unsigned anteli = 0; anteli < ante_clause.literal_count; ++anteli) {
				auto& l = ante_clause.literals[anteli];
				Literal lit(l);
				if (!seen_vars.count(lit.index())) {
					traverse_vars.push_back(lit.index());
					seen_vars.insert(lit.index());
				}
			}
		}
	}

	std::vector<literal_t> conflict_clause;
	conflict_clause.reserve(conflict_clause_set.size());
	for (auto l : conflict_clause_set) {
		conflict_clause.push_back(l);
		Literal lit(l);
		std::cerr << ' ' << (lit.direction() == VAR_TRUE ? '+' : '-') << lit.index();
	}
	std::cerr << '\n';

	if (conflict_clause.size() < 2) {
		enqueue_implication(conflict_clause.front(), -1);
		new_dlevel = 1;
	}
	add_clause(std::move(conflict_clause), CLAUSE_CONFLICT);
	return new_dlevel < dlevel ? new_dlevel - 1 : dlevel - 1;
}

void
Ersatz::Impl::init_solve()
{
	stats.reset();
	conflicting_clauses.clear();
	implications.clear();
	stack.clear();
	assignment_stack.clear();
	clause_stack.clear();
	outcome = OUTCOME_UNDETERMINED;
	dlevel = 0;

#ifdef ERSATZ_USE_UNSATISFIED_CLAUSES
	unsatisfied_clauses.clear();
	unsatisfied_clauses.increase_size(clauses.size());
#endif

	unassigned_vars.clear();
	unassigned_vars.increase_size(vars.size());
	vsids_init();
	for (unsigned i = 0; i < vars.size(); ++i) {
		unassigned_vars.insert(i, 0);
	}

	restart_init();
}


void
Ersatz::Impl::solve()
{
	currently_solving = true;
	init_solve();
	if (!preprocess()) {
		outcome = OUTCOME_UNSATISFIABLE;
	}
	else {
		solver();
	}
	currently_solving = false;
}

void
Ersatz::Impl::reset_solution()
{
	while (assignment_stack.size() > 0) {
		auto l = assignment_stack.back();
		assignment_stack.pop_back();
		Literal lit(l);
		unset_variable(lit.index());
	}

	while (clause_stack.size() > 0) {
		auto c = clause_stack.back();
		clause_stack.pop_back();
		unsatisfy_clause(c);
	}

	while (stack.size() > 0) {
		stack.pop_back();
	}

}


Ersatz::Ersatz()
	: mPimpl(new Ersatz::Impl())
{
}


Ersatz::~Ersatz()
{
}

void Ersatz::reserve(int num_vars, int num_clauses)
{
	mPimpl->reserve_vars(num_vars);
	mPimpl->reserve_clauses(num_clauses);
}

void
Ersatz::set_num_variables(int num_vars)
{
	mPimpl->set_num_variables(num_vars);
}

int
Ersatz::add_variable()
{
	return mPimpl->add_variable(VAR_PROBLEM);
}

void
Ersatz::add_clause(int* begin, int* end)
{
	mPimpl->add_clause(begin, end);
}

void
Ersatz::solve()
{
	mPimpl->solve();
}

void
Ersatz::reset_solution()
{
	mPimpl->reset_solution();
}

int
Ersatz::num_variables() const
{
	return mPimpl->vars.size();
}

bool
Ersatz::query_assignment(int var) const
{
	return mPimpl->vars[var].value == VAR_TRUE;
}


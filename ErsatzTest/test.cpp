#include "pch.h"
#include "Ersatz.h"
#include <map>
#include <set>


#define DUMP_VARIABLES
#undef DUMP_CLAUSES

struct ij {
	int i, j;
	ij(int i = -1, int j = -1)
		: i(i), j(j)
	{
	}
};


struct ijv {
	int i, j, v;
	ijv(int i = -1, int j = -1, int v = -1)
		: i(i), j(j), v(v)
	{
	}
};

#define T(v) (2*(v)+1)
#define F(v) (2*(v))


void dump_clause(int* begin, int* end)
{
	std::cerr << "Adding clause:";
	for (auto i = begin; i != end; ++i) {
		auto l = *i;
		std::cerr << ' ' << (l & 1 ? '+' : '-') << (l / 2);
	}
	std::cerr << '\n';
}



void
exactly_one_constraint(int* v, int n, Ersatz& solver)
{
    std::vector<int> c;
    c.resize(n+1);
    for (int i = 0; i < n; ++i) {
        c[i] = T(v[i]);
    }
	solver.add_clause(&c[0], &c[n]);
#ifdef DUMP_CLAUSES
	dump_clause(&c[0], &c[n]);
#endif
    for (int i = 0; i + 1 < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            c[0] = F(v[i]);
            c[1] = F(v[j]);
            solver.add_clause(&c[0], &c[2]);
#ifdef DUMP_CLAUSES
			dump_clause(&c[0], &c[2]);
#endif
        }
    }
}


void
at_most_one_constraint(int* v, int n, Ersatz& solver)
{
	int c[2];
	for (int i = 0; i + 1 < n; ++i) {
		for (int j = i + 1; j < n; ++j) {
			c[0] = F(v[i]);
			c[1] = F(v[j]);
			solver.add_clause(&c[0], &c[2]);
#ifdef DUMP_CLAUSES
			dump_clause(&c[0], &c[2]);
#endif
		}
	}
}


void
exactly_one_constraint(Ersatz& solver, int a, int b, int c, int d, int e)
{
    int v[5] = { a, b, c, d, e };
    int con[5];
    for (int i = 0; i < 5; ++i) {
        con[i] = T(v[i]);
    }
    solver.add_clause(&con[0], &con[5]);
    for (int i = 0; i + 1 < 5; ++i) {
        for (int j = i + 1; j < 5; ++j) {
            con[0] = F(v[i]);
            con[1] = F(v[j]);
            solver.add_clause(&con[0], &con[2]);
        }
    }
}

#if 0
TEST(Sudoku, Sudoku4Case) {
	// [1]  2 | 3   4
	//  4   3 | [2] 1
	// -------+-------
	//  3  [1]|  4  2
	//  2   4 |  1 [3]

	const unsigned sOrder = 2;
	const unsigned N = sOrder * sOrder;
	Ersatz solver;
	std::map<int, ijv> var_lookup;
	int vars[N][N][N];
	for (unsigned i = 0; i < N; ++i) {
		for (unsigned j = 0; j < N; ++j) {
			for (unsigned v = 0; v < N; ++v) {
				vars[i][j][v] = solver.add_variable();
				var_lookup[vars[i][j][v]] = ijv(i, j, v);
#ifdef DUMP_VARIABLES
				std::cout << vars[i][j][v] << " = V_" << (i + 1) << '_' << (j + 1) << '_' << (v + 1) << '\n';
#endif
			}
		}
	}

	int constraints[N * N * 4][N];

	for (unsigned i = 0; i < N; ++i) {
		for (unsigned j = 0; j < N; ++j) {
			for (unsigned k = 0; k < N; ++k) {
				unsigned bx = (j / sOrder) * sOrder + (k / sOrder);
				unsigned by = (j % sOrder) * sOrder + (k % sOrder);
				constraints[0 * N * N + i * N + j][k] = vars[i][j][k]; // val
				constraints[1 * N * N + i * N + j][k] = vars[k][i][j]; // row
				constraints[2 * N * N + i * N + j][k] = vars[i][k][j]; // col
				constraints[3 * N * N + i * N + j][k] = vars[bx][by][i]; // box
			}
		}
	}
	for (unsigned i = 0; i < N * N * 4; ++i) {
		int constraint[N];
		for (unsigned j = 0; j < N; ++j) {
			constraint[j] = constraints[i][j] * 2 + 1;
		}
		solver.add_clause(&constraint[0], &constraint[N]);
		for (unsigned j = 0; j < N - 1; ++j) {
			for (unsigned k = j + 1; k < N; ++k) {
				constraint[0] = constraints[i][j] * 2;
				constraint[1] = constraints[i][k] * 2;
				solver.add_clause(&constraint[0], &constraint[2]);
			}
		}
	}

	{
		int constraint[1];
		constraint[0] = vars[0][0][0] * 2 + 1;
		solver.add_clause(&constraint[0], &constraint[1]);
		constraint[0] = vars[2][1][1] * 2 + 1;
		solver.add_clause(&constraint[0], &constraint[1]);
		constraint[0] = vars[1][2][0] * 2 + 1;
		solver.add_clause(&constraint[0], &constraint[1]);
		constraint[0] = vars[3][3][2] * 2 + 1;
		solver.add_clause(&constraint[0], &constraint[1]);
	}
	solver.solve();
}
#endif

#if 0
TEST(Sudoku, SudokuMedium) {
	const unsigned sOrder = 3;
	const unsigned N = sOrder * sOrder;
	Ersatz solver;
	std::map<int, ijv> var_lookup;
	int vars[N][N][N];
	for (unsigned i = 0; i < N; ++i) {
		for (unsigned j = 0; j < N; ++j) {
			for (unsigned v = 0; v < N; ++v) {
				vars[i][j][v] = solver.add_variable();
				var_lookup[vars[i][j][v]] = ijv(i, j, v);
#ifdef DUMP_VARIABLES
				std::cout << vars[i][j][v] << " = V_" << (i + 1) << '_' << (j + 1) << '_' << (v + 1) << '\n';
#endif
			}
		}
	}

	int constraints[N*N*4][N];

	for (unsigned i = 0; i < N; ++i) {
		for (unsigned j = 0; j < N; ++j) {
			for (unsigned k = 0; k < N; ++k) {
				unsigned bx = (j / sOrder) * sOrder + (k / sOrder);
				unsigned by = (j % sOrder) * sOrder + (k % sOrder);
				constraints[0 * N * N + i * N + j][k] = vars[i][j][k]; // val
				constraints[1 * N * N + i * N + j][k] = vars[k][i][j]; // row
				constraints[2 * N * N + i * N + j][k] = vars[i][k][j]; // col
				constraints[3 * N * N + i * N + j][k] = vars[bx][by][i]; // box
			}
		}
	}
	for (unsigned i = 0; i < N*N*4; ++i) {
		int constraint[N];
		for (unsigned j = 0; j < N; ++j) {
			constraint[j] = constraints[i][j] * 2 + 1;
		}
		solver.add_clause(&constraint[0], &constraint[N]);
		for (unsigned j = 0; j < N-1; ++j) {
			for (unsigned k = j + 1; k < N; ++k) {
				constraint[0] = constraints[i][j] * 2;
				constraint[1] = constraints[i][k] * 2;
				solver.add_clause(&constraint[0], &constraint[2]);
			}
		}
	}

	{
		ijv givens[] = {
			ijv(1,1,1), ijv(1,2,2), ijv(1,4,3), ijv(1,9,4),
			ijv(2,1,3), ijv(2,2,5), ijv(2,7,1),
			ijv(3,3,4),
			ijv(4,3,5), ijv(4,4,4), ijv(4,7,2),
			ijv(5,1,6), ijv(5,5,7),
			ijv(6,6,8), ijv(6,8,9),
			ijv(7,3,3), ijv(7,4,1), ijv(7,7,5),
			ijv(8,6,9), ijv(8,8,7),
			ijv(9,5,6), ijv(9,9,8)
		};
		int constraint[1];
		for (auto& g : givens) {
			constraint[0] = vars[g.i-1][g.j-1][g.v-1] * 2 + 1;
			solver.add_clause(&constraint[0], &constraint[1]);
		}
	}
	solver.solve();
}
#endif


#if 0
TEST(Sudoku, SudokuPlatinumBlonde) {
	const unsigned sOrder = 3;
	const unsigned N = sOrder * sOrder;
	Ersatz solver;
	std::map<int, ijv> var_lookup;
	int vars[N][N][N];
	for (unsigned i = 0; i < N; ++i) {
		for (unsigned j = 0; j < N; ++j) {
			for (unsigned v = 0; v < N; ++v) {
				vars[i][j][v] = solver.add_variable();
				var_lookup[vars[i][j][v]] = ijv(i, j, v);
#ifdef DUMP_VARIABLES
				std::cout << vars[i][j][v] << " = V_" << (i + 1) << '_' << (j + 1) << '_' << (v + 1) << '\n';
#endif
			}
		}
	}

	int constraints[N * N * 4][N];

	for (unsigned i = 0; i < N; ++i) {
		for (unsigned j = 0; j < N; ++j) {
			for (unsigned k = 0; k < N; ++k) {
				unsigned bx = (j / sOrder) * sOrder + (k / sOrder);
				unsigned by = (j % sOrder) * sOrder + (k % sOrder);
				constraints[0 * N * N + i * N + j][k] = vars[i][j][k]; // val
				constraints[1 * N * N + i * N + j][k] = vars[k][i][j]; // row
				constraints[2 * N * N + i * N + j][k] = vars[i][k][j]; // col
				constraints[3 * N * N + i * N + j][k] = vars[bx][by][i]; // box
			}
		}
	}
	for (unsigned i = 0; i < N * N * 4; ++i) {
		int constraint[N];
		for (unsigned j = 0; j < N; ++j) {
			constraint[j] = constraints[i][j] * 2 + 1;
		}
		solver.add_clause(&constraint[0], &constraint[N]);
		for (unsigned j = 0; j < N - 1; ++j) {
			for (unsigned k = j + 1; k < N; ++k) {
				constraint[0] = constraints[i][j] * 2;
				constraint[1] = constraints[i][k] * 2;
				solver.add_clause(&constraint[0], &constraint[2]);
			}
		}
	}

	{
		ijv givens[] = {
			ijv(1,8,1), ijv(1,9,2),
			ijv(2,9,3),
			ijv(3,3,2), ijv(3,4,3), ijv(3,7,4),
			ijv(4,3,1), ijv(4,4,8), ijv(4,9,5),
			ijv(5,2,6), ijv(5,5,7), ijv(5,7,8),
			ijv(6,6,9),
			ijv(7,3,8), ijv(7,4,5),
			ijv(8,1,9), ijv(8,5,4), ijv(8,7,5),
			ijv(9,1,4), ijv(9,2,7), ijv(9,6,6)
		};
		int constraint[1];
		for (auto& g : givens) {
			constraint[0] = vars[g.i - 1][g.j - 1][g.v - 1] * 2 + 1;
			solver.add_clause(&constraint[0], &constraint[1]);
		}
	}
	solver.solve();
}
#endif


#if 0
TEST(SudokuVariant, SudokuAdjacentKnight) {
	const unsigned sOrder = 3;
	const unsigned N = sOrder * sOrder;
	Ersatz solver;
	std::map<int, ijv> var_lookup;
	int vars[N][N][N];
	for (unsigned i = 0; i < N; ++i) {
		for (unsigned j = 0; j < N; ++j) {
			for (unsigned v = 0; v < N; ++v) {
				vars[i][j][v] = solver.add_variable();
				var_lookup[vars[i][j][v]] = ijv(i, j, v);
#ifdef DUMP_VARIABLES
				std::cout << vars[i][j][v] << " = V_" << (i + 1) << '_' << (j + 1) << '_' << (v + 1) << '\n';
#endif
			}
		}
	}

	int constraints[N * N * 4][N];

	for (unsigned i = 0; i < N; ++i) {
		for (unsigned j = 0; j < N; ++j) {
			for (unsigned k = 0; k < N; ++k) {
				unsigned bx = (j / sOrder) * sOrder + (k / sOrder);
				unsigned by = (j % sOrder) * sOrder + (k % sOrder);
				constraints[0 * N * N + i * N + j][k] = vars[i][j][k]; // val
				constraints[1 * N * N + i * N + j][k] = vars[k][i][j]; // row
				constraints[2 * N * N + i * N + j][k] = vars[i][k][j]; // col
				constraints[3 * N * N + i * N + j][k] = vars[bx][by][i]; // box
			}
		}
	}
	for (unsigned i = 0; i < N * N * 4; ++i) {
		int constraint[N];
		for (unsigned j = 0; j < N; ++j) {
			constraint[j] = constraints[i][j] * 2 + 1;
		}
		solver.add_clause(&constraint[0], &constraint[N]);
		for (unsigned j = 0; j < N - 1; ++j) {
			for (unsigned k = j + 1; k < N; ++k) {
				constraint[0] = constraints[i][j] * 2;
				constraint[1] = constraints[i][k] * 2;
				solver.add_clause(&constraint[0], &constraint[2]);
			}
		}
	}

	// Adjacency constraints
	// 
	// A -> ~B = ~A \/ ~B
	for (int i = 0; i < N; ++i) {
		for (int j = 0; j < N; ++j) {
			for (int v = 0; v + 1 < N; ++v) {
				int constraint[2];
				constraint[0] = vars[i][j][v] * 2;
				int vv = v + 1;

				for (int ii = i-1; ii <= i+1; ii += 2) {
					if (ii < 0 || ii >= N) {
						continue;
					}
					constraint[1] = vars[ii][j][vv] * 2;
					solver.add_clause(&constraint[0], &constraint[2]);
				}
				for (int jj = j - 1; jj <= j + 1; jj += 2) {
					if (jj < 0 || jj >= N) {
						continue;
					}
					constraint[1] = vars[i][jj][vv] * 2;
					solver.add_clause(&constraint[0], &constraint[2]);
				}
			}
		}
	}

	// Knights tour constraint
	{
		ij knights_tour[8] = {
			ij(-2, -1), ij(-1, -2),
			ij(+2, -1), ij(+1, -2),
			ij(-2, +1), ij(-1, +2),
			ij(+2, +1), ij(+1, +2)
		};
		for (int i = 0; i < N; ++i) {
			for (int j = 0; j < N; ++j) {
				for (auto& offset : knights_tour) {
					int ii = i + offset.i;
					int jj = j + offset.j;
					if (ii < 0 || ii >= N || jj < 0 || jj >= N) {
						continue;
					}
					for (int v = 0; v < N; ++v) {
						int constraint[2];
						constraint[0] = vars[i][j][v] * 2;
						constraint[1] = vars[ii][jj][v] * 2;
						solver.add_clause(&constraint[0], &constraint[2]);
					}
				}
			}
		}

	}



	{
		// 
		ijv givens[] = {
			ijv(3,4,4), ijv(3,6,7),
			ijv(4,3,6), ijv(4,7,5),
			ijv(6,3,4), ijv(6,7,3),
			ijv(7,4,2), ijv(7,6,5)
		};
		int constraint[1];
		for (auto& g : givens) {
			constraint[0] = vars[g.i - 1][g.j - 1][g.v - 1] * 2 + 1;
			solver.add_clause(&constraint[0], &constraint[1]);
		}
	}
	solver.solve();
#if 0
	std::vector<int> solution;
	solution.reserve(N*N);
	auto num_vars = solver.num_variables();
	for (int i = 0; i < num_vars; ++i) {
		if (solver.query_assignment(i)) {
			solution.push_back(i * 2);
		}
	}
	solver.reset_solution();
	solver.add_clause(&*solution.begin(), &*solution.begin() + solution.size());
	solver.solve();
#endif
}
#endif


#if 1
TEST(SudokuVariant, SudokuAdjacentKnightNew) {
	const unsigned sOrder = 3;
	const unsigned N = sOrder * sOrder;
	Ersatz solver;
	std::map<int, ijv> var_lookup;
	int vars[N][N][N];
	for (unsigned i = 0; i < N; ++i) {
		for (unsigned j = 0; j < N; ++j) {
			for (unsigned v = 0; v < N; ++v) {
				vars[i][j][v] = solver.add_variable();
				var_lookup[vars[i][j][v]] = ijv(i, j, v);
#ifdef DUMP_VARIABLES
				std::cout << vars[i][j][v] << " = V_" << (i + 1) << '_' << (j + 1) << '_' << (v + 1) << '\n';
#endif
			}
		}
	}

	int constraints[N * N * 4][N];

	for (unsigned i = 0; i < N; ++i) {
		for (unsigned j = 0; j < N; ++j) {
			for (unsigned k = 0; k < N; ++k) {
				unsigned bx = (j / sOrder) * sOrder + (k / sOrder);
				unsigned by = (j % sOrder) * sOrder + (k % sOrder);
				constraints[0 * N * N + i * N + j][k] = vars[i][j][k]; // val
				constraints[1 * N * N + i * N + j][k] = vars[k][i][j]; // row
				constraints[2 * N * N + i * N + j][k] = vars[i][k][j]; // col
				constraints[3 * N * N + i * N + j][k] = vars[bx][by][i]; // box
			}
		}
	}
	for (unsigned i = 0; i < N * N * 4; ++i) {
		int constraint[N];
		for (unsigned j = 0; j < N; ++j) {
			constraint[j] = constraints[i][j] * 2 + 1;
		}
		solver.add_clause(&constraint[0], &constraint[N]);
		for (unsigned j = 0; j < N - 1; ++j) {
			for (unsigned k = j + 1; k < N; ++k) {
				constraint[0] = constraints[i][j] * 2;
				constraint[1] = constraints[i][k] * 2;
				solver.add_clause(&constraint[0], &constraint[2]);
			}
		}
	}

#if 0
	// Adjacency constraints
	// 
	// A -> ~B = ~A \/ ~B
	for (int i = 0; i < N; ++i) {
		for (int j = 0; j < N; ++j) {
			for (int v = 0; v + 1 < N; ++v) {
				int constraint[2];
				constraint[0] = vars[i][j][v] * 2;
				int vv = v + 1;

				for (int ii = i - 1; ii <= i + 1; ii += 2) {
					if (ii < 0 || ii >= N) {
						continue;
					}
					constraint[1] = vars[ii][j][vv] * 2;
					solver.add_clause(&constraint[0], &constraint[2]);
				}
				for (int jj = j - 1; jj <= j + 1; jj += 2) {
					if (jj < 0 || jj >= N) {
						continue;
					}
					constraint[1] = vars[i][jj][vv] * 2;
					solver.add_clause(&constraint[0], &constraint[2]);
				}
			}
		}
	}
#endif

#if 1
	// Knights tour constraint
	{
		ij knights_tour[8] = {
			ij(-2, -1), ij(-1, -2),
			ij(+2, -1), ij(+1, -2),
			ij(-2, +1), ij(-1, +2),
			ij(+2, +1), ij(+1, +2)
		};
		for (int i = 0; i < N; ++i) {
			for (int j = 0; j < N; ++j) {
				for (auto& offset : knights_tour) {
					int ii = i + offset.i;
					int jj = j + offset.j;
					if (ii < 0 || ii >= N || jj < 0 || jj >= N) {
						continue;
					}
					for (int v = 0; v < N; ++v) {
						int constraint[2];
						constraint[0] = vars[i][j][v] * 2;
						if (v > 0) {
							constraint[1] = vars[ii][jj][v-1] * 2;
							solver.add_clause(&constraint[0], &constraint[2]);
						}
						if (v+1 < N) {
							constraint[1] = vars[ii][jj][v + 1] * 2;
							solver.add_clause(&constraint[0], &constraint[2]);
						}

					}
				}
			}
		}
	}
#endif

#if 0
	// Adjacency constraints
	{
		for (int i = 0; i < N; i += sOrder) {
			for (int j = 0; j < N; ++j) {
				for (int v = 0; v < N; ++v) {
					int constraint1[2], constraint2[2];
					constraint1[0] = vars[i][j][v] * 2;
					constraint2[0] = vars[j][i][v] * 2;
					if (v > 0) {
						constraint1[1] = vars[i + 1][j][v-1] * 2;
						solver.add_clause(&constraint1[0], &constraint1[2]);
						constraint2[1] = vars[j][i+1][v - 1] * 2;
						solver.add_clause(&constraint2[0], &constraint2[2]);
					}
					if (v + 1 < N) {
						constraint1[1] = vars[i + 1][j][v + 1] * 2;
						solver.add_clause(&constraint1[0], &constraint1[2]);
						constraint2[1] = vars[j][i + 1][v + 1] * 2;
						solver.add_clause(&constraint2[0], &constraint2[2]);
					}
				}
			}
		}
	}
#endif


#if 0
	{
		// 
		ijv givens[] = {
			ijv(5, 5, 5),
			ijv(2, 2, 2),
			ijv(2, 5, 3),
			ijv(2, 8, 6),
			ijv(8, 2, 7),
			ijv(8, 8, 9)
#if 0

			ijv(3,4,4), ijv(3,6,7),
			ijv(4,3,6), ijv(4,7,5),
			ijv(6,3,4), ijv(6,7,3),
			ijv(7,4,2), ijv(7,6,5)
#endif
		};
		int constraint[1];
		for (auto& g : givens) {
			constraint[0] = vars[g.i - 1][g.j - 1][g.v - 1] * 2 + 1;
			solver.add_clause(&constraint[0], &constraint[1]);
		}
	}
#endif
	solver.solve();
#if 0
	std::vector<int> solution;
	solution.reserve(N * N);
	auto num_vars = solver.num_variables();
	for (int i = 0; i < num_vars; ++i) {
		if (solver.query_assignment(i)) {
			solution.push_back(i * 2);
		}
	}
	solver.reset_solution();
	solver.add_clause(&*solution.begin(), &*solution.begin() + solution.size());
	solver.solve();
#endif
}
#endif


#if 0
TEST(Logic, Zebra) {
    /*
						  The Zebra Problem

   1.  Five people have five different pets, smoke five different
	   brands of cigarettes, have five different favorite drinks and
	   live in five different houses.
   2.  The Englishman lives in the red house.
   3.  The Spaniard has a dog.
   4.  The Ukranian drinks tea.
   5.  The Norwegian lives in the leftmost house.
   6.  The Japanese smokes Parliaments.
   7.  The Norwegian lives next to the blue house.
   8.  Coffee is drunk in the green house.
   9.  The snail owner smokes Old Gold.
   10. The inhabitant of the yellow house smokes Kools.
   11. The Lucky Strikes smoker drinks orange juice.
   12. Milk is drunk in the middle house.
   13. The green house is immediately to the right of the ivory house.
   14. The Chesterfield smoker lives next door to the fox owner.
   15. The Kools smoker lives next door to where the horse is kept.

   Given these conditions, determine who owns the zebra and who drinks
   water.
*/
	Ersatz solver;

	enum nationality { englishman = 0, japanese, norwegian, spaniard, ukranian };
	const char* nationality_str[] = { "Englishman", "Japanese", "Norwegian", "Spaniard", "Ukranian" };
	enum pet { zebra = 0, dog, horse, fox, snails };
	const char* pet_str[] = { "zebra", "dog", "horse", "fox", "snails" };
	enum house_colour { red = 0, green, ivory, yellow, blue };
	const char* colour_str[] = { "red", "green", "ivory", "yellow", "blue" };
	enum cigarette { oldGold = 0, parliament, kools, lucky, chesterfield };
	const char* cigarette_str[] = { "Old Gold", "Parliament", "Kools", "Lucky", "Chesterfields" };
	enum drink { coffee, tea, orangeJuice, water, milk };
	const char* drink_str[] = { "coffee", "tea", "orange juice", "water", "milk" };

	int nationality_housenum[5][5];
	for (unsigned i = 0; i < 5; ++i) {
	    for (unsigned j = 0; j < 5; ++j) {
			auto v = solver.add_variable();
			nationality_housenum[i][j] = v;
#ifdef DUMP_VARIABLES
			std::cerr << v << ' ' << nationality_str[i] << ' ' << (j+1) << '\n';
#endif
	    }
	}

	int nationality_pet[5][5];
	for (unsigned i = 0; i < 5; ++i) {
	    for (unsigned j = 0; j < 5; ++j) {
			auto v = solver.add_variable();
		nationality_pet[i][j] = v;
#ifdef DUMP_VARIABLES
		std::cerr << v << ' ' << nationality_str[i] << ' ' << pet_str[j] << '\n';
#endif
	    }
	}

	int nationality_housecol[5][5];
	for (unsigned i = 0; i < 5; ++i) {
	    for (unsigned j = 0; j < 5; ++j) {
			auto v = solver.add_variable();
		nationality_housecol[i][j] = v;
#ifdef DUMP_VARIABLES
		std::cerr << v << ' ' << nationality_str[i] << ' ' << colour_str[j] << '\n';
#endif
		}
	}

	int nationality_cigarette[5][5];
	for (unsigned i = 0; i < 5; ++i) {
	    for (unsigned j = 0; j < 5; ++j) {
			auto v = solver.add_variable();
		nationality_cigarette[i][j] = v;
#ifdef DUMP_VARIABLES
		std::cerr << v << ' ' << nationality_str[i] << ' ' << cigarette_str[j] << '\n';
#endif
		}
	}

	int nationality_drink[5][5];
	for (unsigned i = 0; i < 5; ++i) {
	    for (unsigned j = 0; j < 5; ++j) {
			auto v = solver.add_variable();
		nationality_drink[i][j] = v;
#ifdef DUMP_VARIABLES
		std::cerr << v << ' ' << nationality_str[i] << ' ' << drink_str[j] << '\n';
#endif
		}
	}

/* ======================================================================= */

	/* Immediately to the left of */
	int immLeft[5][5];
	for (unsigned i = 0; i < 5; ++i) {
	    for (unsigned j = 0; j < 5; ++j) {
	        if (i == j) {
		    continue;
		}
		immLeft[i][j] = solver.add_variable();
	    }
	}

	/* Next to */
	int nextTo[5][5];
	for (unsigned i = 0; i < 5; ++i) {
	    for (unsigned j = 0; j < 5; ++j) {
	        if (i > j) {
		    nextTo[i][j] = nextTo[j][i];
		}
		else if (i < j) {
		    nextTo[i][j] = solver.add_variable();
		}
	    }
	}

        /* Basic constraints. */

	/* Everyone:
	 * 	- lives in exactly one house,
	 * 	- of exactly one color,
	 * 	- has exactly one pet,
	 * 	- has exactly one favourite drink,
	 * 	- smokes exactly one brand of cigarette.
	 */
	for (unsigned i = 0; i < 5; ++i) {
		exactly_one_constraint(solver, nationality_housenum[i][0], nationality_housenum[i][1], nationality_housenum[i][2], nationality_housenum[i][3], nationality_housenum[i][4]);
		exactly_one_constraint(solver, nationality_housecol[i][0], nationality_housecol[i][1], nationality_housecol[i][2], nationality_housecol[i][3], nationality_housecol[i][4]);
		exactly_one_constraint(solver, nationality_pet[i][0], nationality_pet[i][1], nationality_pet[i][2], nationality_pet[i][3], nationality_pet[i][4]);
		exactly_one_constraint(solver, nationality_drink[i][0], nationality_drink[i][1], nationality_drink[i][2], nationality_drink[i][3], nationality_drink[i][4]);
		exactly_one_constraint(solver, nationality_cigarette[i][0], nationality_cigarette[i][1], nationality_cigarette[i][2], nationality_cigarette[i][3], nationality_cigarette[i][4]);
	}


	/* Other basic constraints about physical location. */

	/* Definition of immLeft */
	for (unsigned i = 0; i < 5; ++i) {
	    for (unsigned j = 0; j < 5; ++j) {
		if (i == j) {
		    continue;
		}
		for (unsigned h0 = 0; h0 < 5; ++h0) {
		    for (unsigned h1 = 0; h1 < 5; ++h1) {
			if (h0 == h1) {
			    continue;
			}
		    int c[3];
			c[0] = F(nationality_housenum[i][h0]);
			c[1] = F(nationality_housenum[j][h1]);
			c[2] = h0 + 1 == h1 ? T(immLeft[i][j]) : F(immLeft[i][j]);
                        solver.add_clause(&c[0], &c[3]);
		    }
		}
	    }
	}

	/* Positive constraints about one house being next to another. */
	for (unsigned i = 0; i < 5; ++i) {
	    for (unsigned j = 0; j < 5; ++j) {
		if (i == j) {
		    continue;
		}
		{
		    int c[2] = { F(immLeft[i][j]), T(nextTo[i][j]) };
		    solver.add_clause(&c[0], &c[2]);
		}
	    }
	}

	/* Negative constraints about one house being next to another. */
	for (unsigned i = 0; i < 5; ++i) {
	    for (unsigned j = 0; j < 5; ++j) {
		if (i >= j) {
		    continue;
		}
		{
		    int c[3] = { T(immLeft[i][j]), T(immLeft[j][i]), F(nextTo[i][j]) };
		    solver.add_clause(&c[0], &c[3]);
		}
	    }
	}

	/* Exactly one person lives in each location. */
	/* Exactly one person lives in each house of a given color. */
	/* Exactly one person has each kind of pet. */
	/* Exactly one person has each kind of drink as a favorite. */
	/* Exactly one person smokes each brand of cigarette. */
	for (unsigned i = 0; i < 5; ++i) {
	    for (unsigned j = 0; j < 5; ++j) {
	        for (unsigned k = 0; k < 5; ++k) {
		    if (j == k) {
			continue;
		    }
		    int c[2];
		    c[0] = F(nationality_housenum[j][i]);
		    c[1] = F(nationality_housenum[k][i]);
		    solver.add_clause(&c[0], &c[2]);
		    c[0] = F(nationality_housecol[j][i]);
		    c[1] = F(nationality_housecol[k][i]);
		    solver.add_clause(&c[0], &c[2]);
		    c[0] = F(nationality_pet[j][i]);
		    c[1] = F(nationality_pet[k][i]);
		    solver.add_clause(&c[0], &c[2]);
		    c[0] = F(nationality_drink[j][i]);
		    c[1] = F(nationality_drink[k][i]);
		    solver.add_clause(&c[0], &c[2]);
		    c[0] = F(nationality_cigarette[j][i]);
		    c[1] = F(nationality_cigarette[k][i]);
		    solver.add_clause(&c[0], &c[2]);
	        }
	    }
	}


	/* 1.  The Englishman lives in the red house. */
	{
	    int c[1] = { T(nationality_housecol[englishman][red]) };
	    solver.add_clause(&c[0], &c[1]);
	}

	/* 2.  The Spaniard has a dog. */
	{
	    int c[1] = { T(nationality_pet[spaniard][dog]) };
	    solver.add_clause(&c[0], &c[1]);
	}

	/* 3.  The Ukranian drinks tea. */
	{
	    int c[1] = { T(nationality_drink[ukranian][tea]) };
	    solver.add_clause(&c[0], &c[1]);
	}

	/* 4.  The Norwegian lives in the leftmost house, i.e., house 0. */
	{
	    int c[1] = { T(nationality_housenum[norwegian][0]) };
	    solver.add_clause(&c[0], &c[1]);
	}

	/* 5.  The Japanese smokes Parliaments. */
	{
	    int c[1] = { T(nationality_cigarette[japanese][parliament]) };
	    solver.add_clause(&c[0], &c[1]);
	}

	/* 6.  The Norwegian lives next to the blue house. */
	{
	    int c[2] = { F(nationality_housecol[norwegian][blue]) };
	    solver.add_clause(&c[0], &c[1]);
	    c[0] = F(nationality_housecol[englishman][blue]);
	    c[1] = T(nextTo[englishman][norwegian]);
	    solver.add_clause(&c[0], &c[2]);
	    c[0] = F(nationality_housecol[japanese][blue]);
	    c[1] = T(nextTo[japanese][norwegian]);
	    solver.add_clause(&c[0], &c[2]);
	    c[0] = F(nationality_housecol[spaniard][blue]);
	    c[1] = T(nextTo[spaniard][norwegian]);
	    solver.add_clause(&c[0], &c[2]);
	    c[0] = F(nationality_housecol[ukranian][blue]);
	    c[1] = T(nextTo[ukranian][norwegian]);
	    solver.add_clause(&c[0], &c[2]);
	}

	/* 7.  Coffee is drunk in the green house. */
	for (int i = 0; i < 5; ++i) {
	    for (int j = 0; j < 5; ++j) {
       	        if (i == j) {
                    continue;
    	        }
		int c[2] = { F(nationality_drink[i][coffee]), F(nationality_housecol[j][green]) };
	        solver.add_clause(&c[0], &c[2]);
	    }
	}

	/* 8.  The snail owner smokes Old Gold. */
	for (int i = 0; i < 5; ++i) {
	    for (int j = 0; j < 5; ++j) {
       	        if (i == j) {
                    continue;
    	        }
		int c[2] = { F(nationality_pet[i][snails]), F(nationality_cigarette[j][oldGold]) };
	        solver.add_clause(&c[0], &c[2]);
	    }
	}

	/* 9.  The inhabitant of the yellow house smokes Kools. */
	for (int i = 0; i < 5; ++i) {
	    for (int j = 0; j < 5; ++j) {
       	        if (i == j) {
                    continue;
    	        }
		int c[2] = { F(nationality_housecol[i][yellow]), F(nationality_cigarette[j][kools]) };
	        solver.add_clause(&c[0], &c[2]);
	    }
	}

	/* 10. The Lucky Strikes smoker drinks orange juice. */
	for (int i = 0; i < 5; ++i) {
	    for (int j = 0; j < 5; ++j) {
       	        if (i == j) {
                    continue;
    	        }
		int c[2] = { F(nationality_drink[i][orangeJuice]), F(nationality_cigarette[j][lucky]) };
	        solver.add_clause(&c[0], &c[2]);
	    }
	}

	/* 11. Milk is drunk in the middle house, i.e., house 2. */
	for (int i = 0; i < 5; ++i) {
	    for (int j = 0; j < 5; ++j) {
       	        if (i == j) {
                    continue;
    	        }
		int c[2] = { F(nationality_drink[i][milk]), F(nationality_housenum[j][2]) };
	        solver.add_clause(&c[0], &c[2]);
	    }
	}

	/* 12. The green house is immediately to the right of the ivory house. */
	for (int i = 0; i < 5; ++i) {
	    for (int j = 0; j < 5; ++j) {
       	        if (i == j) {
                    continue;
    	        }
		int c[3] = { F(nationality_housecol[i][ivory]), F(nationality_housecol[j][green]), T(immLeft[i][j]) };
	        solver.add_clause(&c[0], &c[3]);
	    }
	}

	/* 13. The Chesterfield smoker lives next door to the fox owner. */
	for (int i = 0; i < 5; ++i) {
            {
		int c[2] = { F(nationality_cigarette[i][chesterfield]), F(nationality_pet[i][fox]) };
	        solver.add_clause(&c[0], &c[2]);
	    }
	    for (int j = 0; j < 5; ++j) {
       	        if (i == j) {
                    continue;
    	        }
		int c[3] = { F(nationality_cigarette[i][chesterfield]), F(nationality_pet[j][fox]), T(nextTo[i][j]) };
	        solver.add_clause(&c[0], &c[3]);
	    }
	}

	/* 14. The Kools smoker lives next door to where the horse is kept. */
	for (int i = 0; i < 5; ++i) {
            {
		int c[2] = { F(nationality_cigarette[i][kools]), F(nationality_pet[i][horse]) };
	        solver.add_clause(&c[0], &c[2]);
	    }
	    for (int j = 0; j < 5; ++j) {
       	        if (i == j) {
                    continue;
    	        }
		int c[3] = { F(nationality_cigarette[i][kools]), F(nationality_pet[j][horse]), T(nextTo[i][j]) };
	        solver.add_clause(&c[0], &c[3]);
	    }
	}

	solver.solve();
}
#endif


#if 0
TEST(Logic, SendMoreMoney) {
	enum letter_t { lS, lE, lN, lD, lM, lO, lR, lY, lNumLetters };
	char letters[] = "SENDMORY";
	int vars[lNumLetters][10];
	int carry[4];

	Ersatz solver;
	for (unsigned v = 0; v < lNumLetters; ++v) {
		for (unsigned d = 0; d < 10; ++d) {
			vars[v][d] = solver.add_variable();
#ifdef DUMP_VARIABLES
			std::cerr << letters[v] << d << " = " << vars[v][d] << '\n';
#endif
		}
	}

	for (unsigned v = 0; v < 4; ++v) {
		carry[v] = solver.add_variable();
#ifdef DUMP_VARIABLES
		std::cerr << 'c' << v << " = " << carry[v] << '\n';
#endif
	}

	for (unsigned v = 0; v < lNumLetters; ++v) {
		exactly_one_constraint(&vars[v][0], 10, solver);
	}
	

	for (unsigned d = 0; d < 10; ++d) {
		int l[lNumLetters];
		for (unsigned v = 0; v < lNumLetters; ++v) {
			l[v] = vars[v][d];
		}
		at_most_one_constraint(l, lNumLetters, solver);
	}


	//   SEND
	// + MORE
	// -------
	//  MONEY
	// A + B + Ci = Co*10 + D
	// Ci \/ ~A0 \/ ~B1 \/ ~C0
	// Ci \/ ~A0 \/ ~B1 \/ D1

	// D + E = Y + c0
	for (unsigned d1 = 0; d1 < 10; ++d1) {
		for (unsigned d2 = 0; d2 < 10; ++d2) {
			if (d1 == d2) {
				continue;
			}
			int c[3];
			c[0] = F(vars[lD][d1]);
			c[1] = F(vars[lE][d2]);
			unsigned sum = d1 + d2;
			c[2] = sum > 9 ? T(carry[0]) : F(carry[0]);
			solver.add_clause(&c[0], &c[3]);
#ifdef DUMP_CLAUSES
			dump_clause(&c[0], &c[3]);
#endif
			unsigned dY = vars[lY][sum % 10];
			c[2] = T(dY);
			solver.add_clause(&c[0], &c[3]);
#ifdef DUMP_CLAUSES
			dump_clause(&c[0], &c[3]);
#endif
		}
	}

	// N + R + c0 = E + 10*c1

	for (unsigned cin = 0; cin < 2; ++cin) {
		for (unsigned d1 = 0; d1 < 10; ++d1) {
			for (unsigned d2 = 0; d2 < 10; ++d2) {
				if (d1 == d2) {
					continue;
				}
				int c[4];
				c[0] = cin ? F(carry[0]) : T(carry[0]);
				c[1] = F(vars[lN][d1]);
				c[2] = F(vars[lR][d2]);
				unsigned sum = d1 + d2 + cin;
				c[3] = sum > 9 ? T(carry[1]) : F(carry[1]);
				solver.add_clause(&c[0], &c[4]);
#ifdef DUMP_CLAUSES
				dump_clause(&c[0], &c[4]);
#endif
				unsigned dE = vars[lE][sum % 10];
				c[3] = T(dE);
				solver.add_clause(&c[0], &c[4]);
#ifdef DUMP_CLAUSES
				dump_clause(&c[0], &c[4]);
#endif
			}
		}
	}

	// E + O + c1 = N + 10*c2

	for (unsigned cin = 0; cin < 2; ++cin) {
		for (unsigned d1 = 0; d1 < 10; ++d1) {
			for (unsigned d2 = 0; d2 < 10; ++d2) {
				if (d1 == d2) {
					continue;
				}
				int c[4];
				c[0] = cin ? F(carry[1]) : T(carry[1]);
				c[1] = F(vars[lE][d1]);
				c[2] = F(vars[lO][d2]);
				unsigned sum = d1 + d2 + cin;
				c[3] = sum > 9 ? T(carry[2]) : F(carry[2]);
				solver.add_clause(&c[0], &c[4]);
#ifdef DUMP_CLAUSES
				dump_clause(&c[0], &c[4]);
#endif
				unsigned dN = vars[lN][sum % 10];
				c[3] = T(dN);
				solver.add_clause(&c[0], &c[4]);
#ifdef DUMP_CLAUSES
				dump_clause(&c[0], &c[4]);
#endif
			}
		}
	}

	// S + M + c2 = O + 10*c3

	for (unsigned cin = 0; cin < 2; ++cin) {
		for (unsigned d1 = 0; d1 < 10; ++d1) {
			for (unsigned d2 = 0; d2 < 10; ++d2) {
				if (d1 == d2) {
					continue;
				}
				int c[4];
				c[0] = cin ? F(carry[2]) : T(carry[2]);
				c[1] = F(vars[lS][d1]);
				c[2] = F(vars[lM][d2]);
				unsigned sum = d1 + d2 + cin;
				c[3] = sum > 9 ? T(carry[3]) : F(carry[3]);
				solver.add_clause(&c[0], &c[4]);
#ifdef DUMP_CLAUSES
				dump_clause(&c[0], &c[4]);
#endif
				unsigned dO = vars[lO][sum % 10];
				c[3] = T(dO);
				solver.add_clause(&c[0], &c[4]);
#ifdef DUMP_CLAUSES
				dump_clause(&c[0], &c[4]);
#endif
			}
		}
	}


	// c3 = M
	{
		int c[2];

		// ~c3 -> M0
		c[0] = T(carry[3]);
		c[1] = T(vars[lM][0]);
		solver.add_clause(&c[0], &c[2]);
#ifdef DUMP_CLAUSES
		dump_clause(&c[0], &c[2]);
#endif

		// c3 -> M1
		c[0] = F(carry[3]);
		c[1] = T(vars[lM][1]);
		solver.add_clause(&c[0], &c[2]);
#ifdef DUMP_CLAUSES
		dump_clause(&c[0], &c[2]);
#endif
	}

	//. S /= 0, M /= 0
	{
		int c[1];
		c[0] = F(vars[lS][0]);
		solver.add_clause(&c[0], &c[1]);
#ifdef DUMP_CLAUSES
		dump_clause(&c[0], &c[1]);
#endif

		c[0] = F(vars[lM][0]);
		solver.add_clause(&c[0], &c[1]);
#ifdef DUMP_CLAUSES
		dump_clause(&c[0], &c[1]);
#endif
	}

	solver.solve();
}
#endif


#if 0
TEST(HCP, SNm_124) {
	static const unsigned edges[][2] = {
{ 1, 3 }, { 1, 5 }, { 1, 122 }, { 2, 3 }, { 2, 6 }, { 2, 121 }, { 3, 1 },
{ 3, 2 }, { 3, 4 }, { 4, 3 }, { 4, 8 }, { 4, 124 }, { 5, 1 }, { 5, 7 },
{ 5, 9 }, { 6, 2 }, { 6, 7 }, { 6, 10 }, { 7, 5 }, { 7, 6 }, { 7, 8 },
{ 8, 4 }, { 8, 7 }, { 8, 12 }, { 9, 5 }, { 9, 11 }, { 9, 13 }, { 10, 6 },
{ 10, 11 }, { 10, 14 }, { 11, 9 }, { 11, 10 }, { 11, 12 }, { 12, 8 }, { 12, 11 },
{ 12, 16 }, { 13, 9 }, { 13, 15 }, { 13, 17 }, { 14, 10 }, { 14, 15 }, { 14, 18 },
{ 15, 13 }, { 15, 14 }, { 15, 16 }, { 16, 12 }, { 16, 15 }, { 16, 20 }, { 17, 13 },
{ 17, 19 }, { 17, 21 }, { 18, 14 }, { 18, 19 }, { 18, 22 }, { 19, 17 }, { 19, 18 },
{ 19, 20 }, { 20, 16 }, { 20, 19 }, { 20, 24 }, { 21, 17 }, { 21, 23 }, { 21, 25 },
{ 22, 18 }, { 22, 23 }, { 22, 26 }, { 23, 21 }, { 23, 22 }, { 23, 24 }, { 24, 20 },
{ 24, 23 }, { 24, 28 }, { 25, 21 }, { 25, 27 }, { 25, 29 }, { 26, 22 }, { 26, 27 },
{ 26, 30 }, { 27, 25 }, { 27, 26 }, { 27, 28 }, { 28, 24 }, { 28, 27 }, { 28, 32 },
{ 29, 25 }, { 29, 31 }, { 29, 33 }, { 30, 26 }, { 30, 31 }, { 30, 34 }, { 31, 29 },
{ 31, 30 }, { 31, 32 }, { 32, 28 }, { 32, 31 }, { 32, 36 }, { 33, 29 }, { 33, 35 },
{ 33, 37 }, { 34, 30 }, { 34, 35 }, { 34, 38 }, { 35, 33 }, { 35, 34 }, { 35, 36 },
{ 36, 32 }, { 36, 35 }, { 36, 40 }, { 37, 33 }, { 37, 39 }, { 37, 41 }, { 38, 34 },
{ 38, 39 }, { 38, 42 }, { 39, 37 }, { 39, 38 }, { 39, 40 }, { 40, 36 }, { 40, 39 },
{ 40, 44 }, { 41, 37 }, { 41, 43 }, { 41, 45 }, { 42, 38 }, { 42, 43 }, { 42, 46 },
{ 43, 41 }, { 43, 42 }, { 43, 44 }, { 44, 40 }, { 44, 43 }, { 44, 48 }, { 45, 41 },
{ 45, 47 }, { 45, 49 }, { 46, 42 }, { 46, 47 }, { 46, 50 }, { 47, 45 }, { 47, 46 },
{ 47, 48 }, { 48, 44 }, { 48, 47 }, { 48, 52 }, { 49, 45 }, { 49, 51 }, { 49, 53 },
{ 50, 46 }, { 50, 51 }, { 50, 54 }, { 51, 49 }, { 51, 50 }, { 51, 52 }, { 52, 48 },
{ 52, 51 }, { 52, 56 }, { 53, 49 }, { 53, 55 }, { 53, 57 }, { 54, 50 }, { 54, 55 },
{ 54, 58 }, { 55, 53 }, { 55, 54 }, { 55, 56 }, { 56, 52 }, { 56, 55 }, { 56, 60 },
{ 57, 53 }, { 57, 59 }, { 57, 61 }, { 58, 54 }, { 58, 59 }, { 58, 62 }, { 59, 57 },
{ 59, 58 }, { 59, 60 }, { 60, 56 }, { 60, 59 }, { 60, 64 }, { 61, 57 }, { 61, 63 },
{ 61, 65 }, { 62, 58 }, { 62, 63 }, { 62, 66 }, { 63, 61 }, { 63, 62 }, { 63, 64 },
{ 64, 60 }, { 64, 63 }, { 64, 68 }, { 65, 61 }, { 65, 67 }, { 65, 69 }, { 66, 62 },
{ 66, 67 }, { 66, 70 }, { 67, 65 }, { 67, 66 }, { 67, 68 }, { 68, 64 }, { 68, 67 },
{ 68, 72 }, { 69, 65 }, { 69, 71 }, { 69, 73 }, { 70, 66 }, { 70, 71 }, { 70, 74 },
{ 71, 69 }, { 71, 70 }, { 71, 72 }, { 72, 68 }, { 72, 71 }, { 72, 76 }, { 73, 69 },
{ 73, 75 }, { 73, 77 }, { 74, 70 }, { 74, 75 }, { 74, 78 }, { 75, 73 }, { 75, 74 },
{ 75, 76 }, { 76, 72 }, { 76, 75 }, { 76, 80 }, { 77, 73 }, { 77, 79 }, { 77, 81 },
{ 78, 74 }, { 78, 79 }, { 78, 82 }, { 79, 77 }, { 79, 78 }, { 79, 80 }, { 80, 76 },
{ 80, 79 }, { 80, 84 }, { 81, 77 }, { 81, 83 }, { 81, 85 }, { 82, 78 }, { 82, 83 },
{ 82, 86 }, { 83, 81 }, { 83, 82 }, { 83, 84 }, { 84, 80 }, { 84, 83 }, { 84, 88 },
{ 85, 81 }, { 85, 87 }, { 85, 89 }, { 86, 82 }, { 86, 87 }, { 86, 90 }, { 87, 85 },
{ 87, 86 }, { 87, 88 }, { 88, 84 }, { 88, 87 }, { 88, 92 }, { 89, 85 }, { 89, 91 },
{ 89, 93 }, { 90, 86 }, { 90, 91 }, { 90, 94 }, { 91, 89 }, { 91, 90 }, { 91, 92 },
{ 92, 88 }, { 92, 91 }, { 92, 96 }, { 93, 89 }, { 93, 95 }, { 93, 97 }, { 94, 90 },
{ 94, 95 }, { 94, 98 }, { 95, 93 }, { 95, 94 }, { 95, 96 }, { 96, 92 }, { 96, 95 },
{ 96, 100 }, { 97, 93 }, { 97, 99 }, { 97, 101 }, { 98, 94 }, { 98, 99 }, { 98, 102 },
{ 99, 97 }, { 99, 98 }, { 99, 100 }, { 100, 96 }, { 100, 99 }, { 100, 104 }, { 101, 97 },
{ 101, 103 }, { 101, 105 }, { 102, 98 }, { 102, 103 }, { 102, 106 }, { 103, 101 }, { 103, 102 },
{ 103, 104 }, { 104, 100 }, { 104, 103 }, { 104, 108 }, { 105, 101 }, { 105, 107 }, { 105, 109 },
{ 106, 102 }, { 106, 107 }, { 106, 110 }, { 107, 105 }, { 107, 106 }, { 107, 108 }, { 108, 104 },
{ 108, 107 }, { 108, 112 }, { 109, 105 }, { 109, 111 }, { 109, 113 }, { 110, 106 }, { 110, 111 },
{ 110, 114 }, { 111, 109 }, { 111, 110 }, { 111, 112 }, { 112, 108 }, { 112, 111 }, { 112, 116 },
{ 113, 109 }, { 113, 115 }, { 113, 117 }, { 114, 110 }, { 114, 115 }, { 114, 118 }, { 115, 113 },
{ 115, 114 }, { 115, 116 }, { 116, 112 }, { 116, 115 }, { 116, 120 }, { 117, 113 }, { 117, 119 },
{ 117, 121 }, { 118, 114 }, { 118, 119 }, { 118, 122 }, { 119, 117 }, { 119, 118 }, { 119, 120 },
{ 120, 116 }, { 120, 119 }, { 120, 124 }, { 121, 2 }, { 121, 117 }, { 121, 123 }, { 122, 1 },
{ 122, 118 }, { 122, 123 }, { 123, 121 }, { 123, 122 }, { 123, 124 }, { 124, 4 }, { 124, 120 },
{ 124, 123 }, { 20, 68 }, { 68, 20 }
	};

	const unsigned nodes = 124;
	int vars[nodes][nodes];

	Ersatz solver;
	for (unsigned i = 0; i < nodes; ++i) {
		for (unsigned j = 0; j < nodes; ++j) {
			vars[i][j] = solver.add_variable();
#ifdef DUMP_VARIABLES
			std::cout << "x_" << (i + 1) << '_' << (j + 1) << " = " << vars[i][j] << '\n';
#endif
		}
	}

	// Every node must appear on the path, every position must be occupied
	for (unsigned j = 0; j < nodes; ++j) {
		int c1[nodes], c2[nodes];
		for (unsigned i = 0; i < nodes; ++i) {
			c1[i] = T(vars[i][j]);
			c2[i] = T(vars[j][i]);
		}
		solver.add_clause(&c1[0], &c1[nodes]);
		solver.add_clause(&c2[0], &c2[nodes]);

#ifdef DUMP_CLAUSES
		dump_clause(&c1[0], &c1[nodes]);
		dump_clause(&c2[0], &c2[nodes]);
#endif
	}

	// No node appears on the path twice, no distinct nodes occupy the same position
	for (unsigned i = 0; i < nodes; ++i) {
		for (unsigned j = 0; j < nodes; ++j) {
			for (unsigned k = 0; k < nodes; ++k) {
				if (i == k) {
					continue;
				}
				int c[2];

				c[0] = F(vars[i][j]);
				c[1] = F(vars[k][j]);
				solver.add_clause(&c[0], &c[2]);
#ifdef DUMP_CLAUSES
				dump_clause(&c[0], &c[2]);
#endif

				c[0] = F(vars[j][i]);
				c[1] = F(vars[j][k]);
				solver.add_clause(&c[0], &c[2]);
#ifdef DUMP_CLAUSES
				dump_clause(&c[0], &c[2]);
#endif
			}
		}
	}

	// Nonadjacent nodes cannot be adjacent on the path
	std::set<std::pair<unsigned, unsigned>> non_edges;
	for (unsigned i = 0; i < nodes; ++i) {
		for (unsigned j = 0; j < nodes; ++j) {
			if (i == j) {
				continue;
			}
			non_edges.emplace(i, j);
		}
	}
	for (auto& edge : edges) {
		non_edges.erase(std::make_pair<unsigned, unsigned>(edge[0] - 1, edge[1] - 1));
	}
	for (auto& ne : non_edges) {
		auto i = ne.first;
		auto j = ne.second;
		for (unsigned k = 0; k < nodes; ++k) {
			unsigned k1 = k + 1 == nodes ? 0 : k + 1;
			int c[2] = { F(vars[k][i]), F(vars[k1][j]) };
			solver.add_clause(&c[0], &c[2]);
#ifdef DUMP_CLAUSES
			dump_clause(&c[0], &c[2]);
#endif
		}
	}

#if 0
	// Start at node 1. Why not.
	{
		int c[1] = { T(vars[0][0]) };
		solver.add_clause(&c[0], &c[1]);
#ifdef DUMP_CLAUSES
		dump_clause(&c[0], &c[1]);
#endif
	}
#endif

	solver.solve();
}
#endif
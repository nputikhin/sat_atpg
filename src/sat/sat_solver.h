#pragma once

#include "../cnf.h"

#include <memory>

class SatSolver
{
public:
	enum SolveStatus
	{
		Sat,
		Unsat,
		Unknown
	};
	virtual SolveStatus solve(const Cnf& cnf) = 0;

	virtual void set_max_lit(literal_t lit) = 0;
	virtual void reset() = 0;
	virtual void add_clause(const clause_t& clause) = 0;
	virtual void add_clause(literal_t l1, literal_t l2 = 0, literal_t l3 = 0, literal_t l4 = 0, literal_t l5 = 0) = 0;
	virtual SolveStatus solve_prepared() = 0;

	virtual int8_t get_value(literal_t l) = 0;
};

namespace SolverFactory
{
	std::unique_ptr<SatSolver> make_solver();
}


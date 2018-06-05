#pragma once

#include "sat_solver.h"

#include <memory>

namespace CaDiCaL
{
	class Solver;
}

class CadicalSolver : public SatSolver
{
public:
	CadicalSolver();

	SolveStatus solve(const Cnf& cnf) override;

	void set_max_lit(literal_t lit) override;
	void reset() override;
	void add_clause(const clause_t& clause) override;
	void add_clause(literal_t l1, literal_t l2 = 0, literal_t l3 = 0, literal_t l4 = 0, literal_t l5 = 0) override;
	SolveStatus solve_prepared() override;

	int8_t get_value(literal_t l) override;

private:
	void reset_solver();

	std::shared_ptr<CaDiCaL::Solver> m_solver;
};

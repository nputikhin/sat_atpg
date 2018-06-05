#pragma once

#include "cnf.h"
#include "sat/sat_solver.h"

class ProxyCnf : public ICnf
{
public:
	ProxyCnf(SatSolver& solver)
		: m_solver(solver)
	{}

	ICnf& operator=(const Cnf& other) override final
	{
		m_solver.reset();
		for (const auto& clause : other.get_clauses()) {
			m_solver.add_clause(clause);
		}
		return *this;
	}

	void reserve(size_t size) override final
	{
		m_solver.set_max_lit(size);
	}

	void clear() override final
	{
		m_solver.reset();
	}

	void add_clause(clause_t clause) override final
	{
		m_solver.add_clause(clause);
	}

	void add_clause(literal_t l1, literal_t l2 = 0, literal_t l3 = 0, literal_t l4 = 0, literal_t l5 = 0) override final
	{
		m_solver.add_clause(l1, l2, l3, l4, l5);
	}

	void add_clauses(std::vector<clause_t>& from) override final
	{
		for (const auto& clause : from) {
			m_solver.add_clause(clause);
		}
	}

private:
	SatSolver& m_solver;
};


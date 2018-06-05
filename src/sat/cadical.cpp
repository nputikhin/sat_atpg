#include "cadical.h"

#include "../util/timer.h"
#include "../util/log.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <cadical.hpp>
#pragma GCC diagnostic pop

#include <cassert>

CadicalSolver::CadicalSolver()
{
	reset_solver();
}

CadicalSolver::SolveStatus CadicalSolver::solve(const Cnf& cnf)
{
	reset_solver();
	for (const auto& clause : cnf.get_clauses()) {
		for (literal_t l : clause) {
			m_solver->add(l);
		}
		m_solver->add(0);
	}

	int cadical_status = m_solver->solve();

	SolveStatus status = SolveStatus::Unknown;
	if (cadical_status == 10)
		status = SolveStatus::Sat;
	else if (cadical_status == 20)
		status = SolveStatus::Unsat;
	return status;
}

void CadicalSolver::set_max_lit(literal_t lit)
{
	m_solver->init(lit);
}

void CadicalSolver::reset()
{
	reset_solver();
}

void CadicalSolver::add_clause(const clause_t& clause)
{
	for (literal_t l : clause) {
		m_solver->add(l);
	}
	m_solver->add(0);
}

void CadicalSolver::add_clause(literal_t l1, literal_t l2, literal_t l3, literal_t l4, literal_t l5)
{
	assert(l1);
	m_solver->add(l1);

	for (literal_t l : {l2, l3, l4, l5}) {
		if (!l)
			break;

		m_solver->add(l);
	}

	m_solver->add(0);
}

CadicalSolver::SolveStatus CadicalSolver::solve_prepared()
{
	int cadical_status = m_solver->solve();

	SolveStatus status = SolveStatus::Unknown;
	if (cadical_status == 10)
		status = SolveStatus::Sat;
	else if (cadical_status == 20)
		status = SolveStatus::Unsat;
	return status;
}


int8_t CadicalSolver::get_value(literal_t l)
{
	return m_solver->val(l);
}

void CadicalSolver::reset_solver()
{
	m_solver.reset(new CaDiCaL::Solver());
	assert(m_solver);
	m_solver->set("quiet", true);
	m_solver->set("rephase", false);
	m_solver->set("profile", 0);
	m_solver->set("restartint", 400);
}

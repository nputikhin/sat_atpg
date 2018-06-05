#include "cnf.h"

#include "util/log.h"

#include <algorithm>
#include <sstream>
#include <cassert>

bool is_true(literal_t l)
{
	return l > 0;
}

void Cnf::add_clause(clause_t clause)
{
	m_clauses.push_back(clause);
}

void Cnf::add_clause(literal_t l1, literal_t l2, literal_t l3, literal_t l4, literal_t l5)
{
	assert(l1);

	clause_t clause = {l1};

	for (literal_t l : {l2, l3, l4, l5}) {
		if (!l)
			break;

		clause.push_back(l);
	}

	m_clauses.push_back(clause);
}

void Cnf::add_clauses(std::vector<clause_t>& from)
{
	m_clauses.insert(m_clauses.end(),
			std::make_move_iterator(from.begin()),
			std::make_move_iterator(from.end()));
}

bool Cnf::is_satisfied(const assignment_t& assignment) const
{
	for (const auto& clause : m_clauses) {
		bool clause_satisfied = false;
		for (literal_t l : clause) {
			if (assignment.at(std::abs(l)) == is_true(l)) {
				clause_satisfied = true;
				break;
			}
		}
		if (!clause_satisfied) {
			return false;
		}
	}
	return true;
}

std::string Cnf::get_dimacs_str() const
{
	size_t num_clauses = m_clauses.size();
	literal_t num_literals = 0;
	for (const clause_t& clause : m_clauses) {
		for (literal_t l : clause) {
			num_literals = std::max(std::abs(l), num_literals);
		}
	}

	std::stringstream ss;
	ss << "p cnf " << num_literals << " " << num_clauses << "\n";

	for (const clause_t& clause : m_clauses) {
		for (literal_t l : clause) {
			ss << l << " ";
		}
		ss << "0\n";
	}
	return ss.str();
}


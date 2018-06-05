#pragma once

#include "circuit_graph.h"
#include "cnf.h"

literal_t line_to_literal(size_t id);
size_t literal_to_line(literal_t l);

class CircuitToCnfTransformer
{
public:
	Cnf make_cnf(const CircuitGraph& graph, bool expand_gates = false);
	static std::vector<clause_t> make_clauses(const Gate& gate);
};

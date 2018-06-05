#pragma once

#include "circuit_graph.h"
#include "fault_cnf.h"

#include <vector>

class FaultManager
{
public:
	FaultManager(const CircuitGraph& circuit);

	bool has_faults_left();
	Fault next_fault();

private:
	void add_stem_fault(const Line& line);
	void add_gate_input_fault(const Line& line, const Line::Connection& connection, bool is_stem);

	std::vector<Fault> m_faults;
	std::vector<Fault>::iterator m_it;
};

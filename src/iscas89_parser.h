#pragma once

#include "circuit_graph.h"

#include <iostream>

class Iscas89Parser
{
public:
	bool parse(std::istream& is, CircuitGraph& graph);

private:
	bool match_input(CircuitGraph& graph, const std::string& line);
	bool match_output(CircuitGraph& graph, const std::string& line);
	bool match_gate(CircuitGraph& graph, const std::string& line);
};

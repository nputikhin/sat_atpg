#pragma once

#include "cnf.h"
#include "circuit_graph.h"
#include "circuit_to_cnf.h"

#include "util/log.h"

struct Fault
{
	Fault() = default;
	Fault(const Line* line, int8_t stuck_at, bool is_stem, Gate* dest = nullptr, size_t input_idx = std::numeric_limits<size_t>::max(), bool is_primary_output = false)
		: line(line)
		, stuck_at(stuck_at)
		, is_stem(is_stem)
		, is_primary_output(is_primary_output)
	{
		connection.gate = dest;
		connection.input_idx = input_idx;
	}

	bool operator==(const Fault& other) const
	{
		return std::tie(line, stuck_at, is_stem, connection) == std::tie(other.line, other.stuck_at, other.is_stem, other.connection);
	}

	bool operator!=(const Fault& other) const
	{
		return !operator==(other);
	}

	const Line* line = nullptr;
	int8_t stuck_at = -1;
	bool is_stem = false; // if true, fanout stem fault
	Line::Connection connection = {}; // if is_stem is false, specifies connection to gate which uses this line as input (fanout branch fault)
	bool is_primary_output = false; // if is_stem is false, specifies that this fault is only for primary output
};

struct FanoutConeInfo
{
	std::set<const Line*> boundary_lines;
	std::set<const Line*> lines_inside;
	std::set<const Line*> primary_outputs_inside;
};

FanoutConeInfo make_fanout_cone(const Fault& fault);

class FaultCnfMaker
{
public:
	FaultCnfMaker(const CircuitGraph& circuit)
		: m_circuit(circuit)
	{}

	void set_threshold_ratio(float threshold_ratio)
	{
		m_threshold_ratio = threshold_ratio;
	}

	void make_fault(Fault fault, ICnf& cnf);
	bool make_and_solve_fault(Fault fault);

private:
	void add_sensitization(ICnf& cnf, const FanoutConeInfo& fanout_cone);
	void add_fault_activation(ICnf& cnf);
	void add_boundary_scan(ICnf& cnf, const FanoutConeInfo& fanout_cone);
	void add_fault_presentation(ICnf& cnf, const FanoutConeInfo& fanout_cone);

	void add_gate_sensitization(ICnf& cnf, const Gate& gate, bool use_spec_x, bool use_spec_y);
	void add_gate_sensitization_with_expansion(ICnf& cnf, const Line::Connection& connection);

	literal_t get_sensitization_lit(const Line* line);
	literal_t get_lit(const Line* line);

	std::string named_clause_str(const clause_t& clause);

	struct Context
	{
		Fault fault = {};
		std::vector<literal_t> line_to_sensitization_literal;
		std::unordered_map<literal_t, const Line*> literal_to_line;
		std::unordered_map<literal_t, const Line*> sensitization_literal_to_line;

		literal_t spec_lit = 0;

		literal_t max_literal = 0;

		void init(const CircuitGraph& circuit, const Fault& fault)
		{
			this->fault = fault;
			line_to_sensitization_literal.resize(circuit.line_id_end(), 0);
			max_literal = line_to_literal(circuit.line_id_end());
		}

		literal_t get_spec_lit()
		{
			if (!spec_lit) {
				spec_lit = make_new_lit();
			}
			return spec_lit;
		}

		literal_t make_new_lit()
		{
			assert(max_literal);
			return max_literal++;
		}

		bool valid()
		{
			return !!fault.line;
		}

		void reset()
		{
			fault = {};
			line_to_sensitization_literal.clear();
			literal_to_line.clear();
			sensitization_literal_to_line.clear();
			spec_lit = 0;
			max_literal = 0;
		}
	};

	Context m_context;
	const CircuitGraph& m_circuit;
	Cnf m_circuit_cnf;
	double m_threshold_ratio = 0.6;
};

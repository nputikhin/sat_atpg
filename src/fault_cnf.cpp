#include "fault_cnf.h"

#include "util/log.h"

#include <unordered_set>
#include <cassert>
#include <sstream>

FanoutConeInfo make_fanout_cone(const Fault& fault)
{
	FanoutConeInfo fanout_cone;
	fanout_cone.lines_inside.insert(fault.line);

	const auto& line_out_gates = fault.line->destination_gates;

	std::vector<const Gate*> source_gates;

	if (fault.is_stem)
	{
		if (!line_out_gates.empty()) {
			source_gates.insert(source_gates.end(), line_out_gates.begin(), line_out_gates.end());
		} else {
			assert(fault.line->is_output);
			fanout_cone.primary_outputs_inside.insert(fault.line);
		}
	} else if (fault.is_primary_output) {
		assert(fault.line->is_output);
		fanout_cone.primary_outputs_inside.insert(fault.line);
	} else {
		source_gates.push_back(fault.connection.gate);
	}

	if (source_gates.empty()) {
		return fanout_cone;
	}

	auto process_gate = [&fanout_cone, &fault](const Gate* gate) {
		for (Line* input : gate->get_inputs()) {
			bool add_fault_line_as_boundary = (input == fault.line) && (!fault.is_stem) && (gate != fault.connection.gate);
			if (add_fault_line_as_boundary || !fanout_cone.lines_inside.count(input)) {
				fanout_cone.boundary_lines.insert(input);
			}
		}
		Line* output = gate->get_output();
		fanout_cone.lines_inside.insert(output);
		fanout_cone.boundary_lines.erase(output);
		if (output->is_output) {
			fanout_cone.primary_outputs_inside.insert(output);
		}
	};

	walk_gates_breadth_first(source_gates, process_gate, true, true);

	return fanout_cone;
}

void FaultCnfMaker::make_fault(Fault fault, ICnf& cnf)
{
	cnf.clear();
	m_context.init(m_circuit, fault);

	size_t output_size_threshold = m_circuit.get_outputs().size() * m_threshold_ratio;

	FanoutConeInfo fanout_cone = make_fanout_cone(m_context.fault);

	if (fanout_cone.primary_outputs_inside.size() < output_size_threshold) {
		std::vector<const Gate*> out_gates;

		out_gates.reserve(fanout_cone.primary_outputs_inside.size());
		for (const Line* l : fanout_cone.primary_outputs_inside) {
			if (l->source)
				out_gates.push_back(l->source);
		}

		auto add_gate_to_cnf = [&cnf](const Gate* gate) {
			auto gate_clauses = CircuitToCnfTransformer::make_clauses(*gate);
			cnf.add_clauses(gate_clauses);
		};
		walk_gates_breadth_first(out_gates, add_gate_to_cnf, false, true);
	} else {
		if (m_circuit_cnf.get_clauses().empty()) {
			CircuitToCnfTransformer transfromer;
			m_circuit_cnf = transfromer.make_cnf(m_circuit, true);
		}
		cnf = m_circuit_cnf;
	}

	// Sensitization clause set
	add_sensitization(cnf, fanout_cone);

	// Fault activation clause set
	add_fault_activation(cnf);

	// Boundary scan clause set
	add_boundary_scan(cnf, fanout_cone);

	// Fault presentation clause set
	add_fault_presentation(cnf, fanout_cone);

	m_context.reset();
}

/*
	Sensitization constraints for (N)AND and (N)OR gates:
	- phi_1: If any input of a gate assumes a controlling value and is not sensitized, then
	the gate output is not sensitized, i.e., x_s = 0.
	- phi_2: If all inputs of a gate are not sensitized, then the gate output is not sensitized.
	- phi_3: If any two inputs are sensitized but their logic values differ, then the output is
	not sensitized.
	- phi_4: For any two inputs of a gate, if one is not sensitized with a non-controlling value
	while the other one is sensitized, then the output is sensitized.
	- phi_5: For any two inputs of a gate, if both are sensitized and their logic value is the
	same, then the output is sensitized
*/

// Sensitization for z = (N)AND(x, y) gate
void add_and_sensitization(ICnf& cnf, literal_t x, literal_t x_s, literal_t y, literal_t y_s, literal_t z_s)
{
	cnf.add_clause(     x_s,      y_s, -z_s);
	cnf.add_clause(    -x_s, -y,  y_s,  z_s);
	cnf.add_clause(-x,  x_s,     -y_s,  z_s);
	cnf.add_clause( x, -x_s,  y, -y_s,  z_s);
	cnf.add_clause(-x, -x_s,  y,       -z_s);
	cnf.add_clause( x,  x_s,           -z_s);
	cnf.add_clause( x,       -y, -y_s, -z_s);
	cnf.add_clause(-x,       -y, -y_s,  z_s);
	cnf.add_clause(           y,  y_s, -z_s);
}

// Sensitization for z = (N)OR(x, y) gate
void add_or_sensitization(ICnf& cnf, literal_t x, literal_t x_s, literal_t y, literal_t y_s, literal_t z_s)
{
	cnf.add_clause(     x_s,      y_s, -z_s);
	cnf.add_clause(    -x_s,  y,  y_s,  z_s);
	cnf.add_clause( x,  x_s,     -y_s,  z_s);
	cnf.add_clause(-x, -x_s, -y, -y_s,  z_s);
	cnf.add_clause( x, -x_s, -y,       -z_s);
	cnf.add_clause(-x,  x_s,           -z_s);
	cnf.add_clause( x,        y, -y_s,  z_s);
	cnf.add_clause(-x,        y, -y_s, -z_s);
	cnf.add_clause(          -y,  y_s, -z_s);
}

// Sensitization for z = X(N)OR(x, y) gate
void add_xor_sensitization(ICnf& cnf, literal_t x_s, literal_t y_s, literal_t z_s)
{
	cnf.add_clause(-x_s, -y_s, -z_s);
	cnf.add_clause( x_s,  y_s, -z_s);
	cnf.add_clause( x_s, -y_s,  z_s);
	cnf.add_clause(-x_s,  y_s,  z_s);
}

// Propagate sensitization variable for z = BUFF(x) and z = NOT(x) gates
void add_sensitization_propagation(ICnf& cnf, literal_t x_s, literal_t z_s)
{
	cnf.add_clause(-x_s,  z_s);
	cnf.add_clause( x_s, -z_s);
}

void FaultCnfMaker::add_gate_sensitization(ICnf& cnf, const Gate& gate, bool use_spec_x, bool use_spec_y)
{
	if (gate.get_inputs().size() == 1)
	{
		assert(gate.get_type() == Gate::Type::Buff || gate.get_type() == Gate::Type::Not);
		literal_t x_s = use_spec_x ? m_context.get_spec_lit() : get_sensitization_lit(gate.get_inputs().front());
		literal_t z_s = get_sensitization_lit(gate.get_output());
		add_sensitization_propagation(cnf, x_s, z_s);
		return;
	}

	assert(gate.get_inputs().size() == 2);
	literal_t x = get_lit(gate.get_inputs().front());
	literal_t y = get_lit(gate.get_inputs().back());
	literal_t x_s = use_spec_x ? m_context.get_spec_lit() : get_sensitization_lit(gate.get_inputs().front());
	literal_t y_s = use_spec_y ? m_context.get_spec_lit() : get_sensitization_lit(gate.get_inputs().back());
	literal_t z_s = get_sensitization_lit(gate.get_output());

	switch(gate.get_type()) {
		case Gate::Type::Nand:
			//[[fallthrough]];
		case Gate::Type::And:
			add_and_sensitization(cnf, x, x_s, y, y_s, z_s);
			break;
		case Gate::Type::Nor:
			//[[fallthrough]];
		case Gate::Type::Or:
			add_or_sensitization(cnf, x, x_s, y, y_s, z_s);
			break;
		case Gate::Type::Xnor:
			//[[fallthrough]];
		case Gate::Type::Xor:
			add_xor_sensitization(cnf, x_s, y_s, z_s);
			break;
		default:
			assert(false);
			break;
	}
}

void FaultCnfMaker::add_gate_sensitization_with_expansion(ICnf& cnf, const Line::Connection& connection)
{
	size_t inputs_size = connection.gate->get_inputs().size();
	for (size_t i = 0; i < connection.gate->get_expanded().size(); ++i) {
		const Gate* expanded_gate = connection.gate->get_expanded()[i];
		assert(expanded_gate->get_inputs().size() <= 2);

		bool use_spec_x = false;
		bool use_spec_y = false;

		Line* line_x = expanded_gate->get_inputs().front();
		if (line_x == m_context.fault.line) {
			use_spec_x = m_context.fault.connection.gate != connection.gate;
			if (!use_spec_x) {
				size_t inp_idx = inputs_size > 1 ? inputs_size - 2 - i : 0;
				use_spec_x = m_context.fault.connection.input_idx != inp_idx;
			}
		}

		Line* line_y = expanded_gate->get_inputs().size() < 2 ? nullptr : expanded_gate->get_inputs().back();
		if (line_y == m_context.fault.line) {
			use_spec_y = m_context.fault.connection.gate != connection.gate;
			if (!use_spec_y) {
				assert(inputs_size >= 2);
				size_t inp_idx = inputs_size - 2 - i + 1;
				use_spec_y = m_context.fault.connection.input_idx != inp_idx;
			}
		}

		add_gate_sensitization(cnf, *expanded_gate, use_spec_x, use_spec_y);
	}
}

void FaultCnfMaker::add_sensitization(ICnf& cnf, const FanoutConeInfo& fanout_cone)
{
	assert(m_context.valid());
	// Make sensitization variables for each gate
	// in fanout cone of fault site
	// Use gate expansion to sensitize gates with input n > 2

	IdObjectSet<const Gate*> sensitized_gates(m_circuit.gate_id_end());
	if (!m_context.fault.is_stem && !m_context.fault.is_primary_output) {
		assert(m_context.fault.connection.gate);
		sensitized_gates.insert(m_context.fault.connection.gate);
		add_gate_sensitization_with_expansion(cnf, m_context.fault.connection);
	}

	for (const Line* line : fanout_cone.lines_inside) {
		if (!m_context.fault.is_stem && line == m_context.fault.line) {
			continue;
		}
		for (const Line::Connection& connection : line->destinations) {
			if (sensitized_gates.count(connection.gate)) {
				continue;
			}
			sensitized_gates.insert(connection.gate);
			add_gate_sensitization_with_expansion(cnf, connection);
		}
	}
}

void FaultCnfMaker::add_fault_activation(ICnf& cnf)
{
	assert(m_context.valid());
	const Fault& f = m_context.fault;
	cnf.add_clause(get_sensitization_lit(f.line));
	cnf.add_clause((f.stuck_at == 0 ? 1 : -1) * get_lit(f.line));
}

void FaultCnfMaker::add_boundary_scan(ICnf& cnf, const FanoutConeInfo& fanout_cone)
{
	assert(m_context.valid());
	// Clauses relating nodes that are not in transitive fanout of fault site
	// With nodes that are in fanout of those nodes and can propagate the fault
	//
	// Alternative formulation:
	// z is the fault site
	// require x_s = 0 if x is not in the transitive fanout of z but at least one fanout node of x is in the transitive fanout of z
	for (const Line* boundary_line : fanout_cone.boundary_lines) {
		if (boundary_line == m_context.fault.line) {
			cnf.add_clause(-m_context.get_spec_lit());
		} else {
			cnf.add_clause(-get_sensitization_lit(boundary_line));
		}
	}

	if (!m_context.fault.is_stem && m_context.fault.line->has_multiple_outputs_to_gate()) {
		bool need_clause = false;
		bool met_gate = false;
		for (const Line::Connection& c : m_context.fault.line->destinations) {
			if (c.gate != m_context.fault.connection.gate) {
				continue;
			}

			if (!met_gate) {
				met_gate = true;
				continue;
			}

			need_clause = true;
			break;
		}
		if (need_clause) {
			cnf.add_clause(-m_context.get_spec_lit());
		}
	}
}

void FaultCnfMaker::add_fault_presentation(ICnf& cnf, const FanoutConeInfo& fanout_cone)
{
	assert(m_context.valid());
	// Clauses that require at least one of the primary outputs to propagate the fault
	// Alternative formulation:
	// At least one primary output should become sensitized
	clause_t fault_presentation_clause;
	for (const Line* primary_output : fanout_cone.primary_outputs_inside) {
		fault_presentation_clause.push_back(get_sensitization_lit(primary_output));
	}
	cnf.add_clause(fault_presentation_clause);
}

literal_t FaultCnfMaker::get_sensitization_lit(const Line* line)
{
	assert(line->id < m_context.line_to_sensitization_literal.size());
	literal_t& lit = m_context.line_to_sensitization_literal[line->id];
	if (!lit) {
		lit = m_context.make_new_lit();
	}
	return lit;
}

literal_t FaultCnfMaker::get_lit(const Line* line)
{
	return line_to_literal(line->id);
}

std::string FaultCnfMaker::named_clause_str(const clause_t& clause)
{
	std::stringstream ss;
	for (literal_t l : clause) {
		assert(l);
		if (l < 0)
			ss << "-";

		l = std::abs(l);
		if (m_context.literal_to_line.count(l)) {
			ss << m_context.literal_to_line.at(l)->name << "_g";
		} else if (m_context.sensitization_literal_to_line.count(l)) {
			ss << m_context.sensitization_literal_to_line.at(l)->name << "_s";
		} else if (l == m_context.get_spec_lit()) {
			ss << "_special_";
		} else {
			ss << "?" << l;
			log_error() << ss.str();
			assert(false);
		}
		ss << " ";
	}
	return ss.str();
}

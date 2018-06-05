#include "circuit_graph.h"

#include "util/log.h"

#include <sstream>
#include <map>
#include <set>

const char* make_gate_name(Gate::Type type)
{
	switch(type) {
		case Gate::Type::And:
			return "AND";
			break;
		case Gate::Type::Nand:
			return "NAND";
			break;
		case Gate::Type::Not:
			return "NOT";
			break;
		case Gate::Type::Or:
			return "OR";
			break;
		case Gate::Type::Nor:
			return "NOR";
			break;
		case Gate::Type::Xor:
			return "XOR";
			break;
		case Gate::Type::Xnor:
			return "XNOR";
			break;
		case Gate::Type::Buff:
			return "BUFF";
			break;
		case Gate::Type::Undefined:
			return "UNDEFINED";
			break;
	}
	assert(false);
	return "???";
}

Gate::Gate(IdMaker& id_maker, Gate::Type type, Line* output, std::vector<Line*>&& inputs)
	: m_id_maker(id_maker)
	, m_type(type)
	, m_inputs(inputs)
	, m_output(output)
	, m_id(id_maker.gate_make_id())
{

	bool is_expandable = inputs.size() > 2;
	if (!is_expandable) {
		m_expanded_gate_ptrs = { this };
	} else {
		Type top_gate = Type::Undefined;
		Type other_gates = Type::Undefined;
		switch (m_type) {
			case Type::And:
				top_gate = Type::And;
				other_gates = Type::And;
				break;
			case Type::Nand:
				top_gate = Type::Nand;
				other_gates = Type::And;
				break;
			case Type::Or:
				top_gate = Type::Or;
				other_gates = Type::Or;
				break;
			case Type::Nor:
				top_gate = Type::Nor;
				other_gates = Type::Or;
				break;
			default:
				assert(false);
				break;
		}

		Line* second_input = m_inputs.back();
		for (auto it = m_inputs.rbegin() + 1; it != m_inputs.rend(); ++it) {
			bool is_top_gate = it == m_inputs.rend() - 1;

			Line* first_input = *it;

			Type type = is_top_gate ? top_gate : other_gates;

			Line* output = nullptr;
			if (is_top_gate) {
				output = m_output;
			} else {
				m_expanded_lines.emplace_back(id_maker.line_make_id(), true);
				Line& line = m_expanded_lines.back();
				line.name = m_output->name;
				line.name += "_E_";
				line.name += std::to_string(std::distance(m_inputs.rbegin(), it));
				output = &line;
			}

			m_expanded_gate.emplace_back(id_maker, type, output, std::vector<Line*>{first_input, second_input});
			Gate& gate = m_expanded_gate.back();

			m_expanded_gate_ptrs.push_back(&gate);

			second_input = gate.get_output();
		}
	}
}

const std::vector<Gate*>& Gate::get_expanded() const
{
	return m_expanded_gate_ptrs;
}

std::string Gate::get_str() const
{
	std::stringstream ss;
	ss << m_output->name << " = " << make_gate_name(m_type) << "(";
	for (auto it = m_inputs.begin(); it != m_inputs.end() - 1; ++it) {
		ss << (*it)->name << ", ";
	}
	ss << m_inputs.back()->name;
	ss << ")";
	return ss.str();
}

Line* CircuitGraph::add_input(const std::string& name)
{
	Line* p_line = ensure_line(name);

	assert(p_line);

	m_inputs.push_back(p_line);
	return p_line;
}

Line* CircuitGraph::add_output(const std::string& name)
{
	Line* p_line = ensure_line(name);

	assert(p_line);

	if (!p_line->is_output) {
		p_line->is_output = true;
		m_outputs.push_back(p_line);
	}

	return p_line;
}

Gate* CircuitGraph::add_gate(Gate::Type type, const std::vector<std::string>& input_names, const std::string& output_name)
{

	std::vector<Line*> inputs;
	for (size_t i = 0; i < input_names.size(); ++i) {
		const std::string input_name = input_names.at(i);
		Line* p_input = ensure_line(input_name);
		inputs.push_back(p_input);
	}

	Line* p_output = ensure_line(output_name);

	m_gates.emplace_back(*this, type, p_output, std::move(inputs));
	Gate& gate = m_gates.back();

	p_output->source = &gate;

	for (size_t i = 0; i < gate.get_inputs().size(); ++i) {
		gate.get_inputs().at(i)->connect_as_input(&gate, i);
	}

	// Gate validation
	switch(gate.type()) {
		case Gate::Type::And:
		case Gate::Type::Nand:
		case Gate::Type::Or:
		case Gate::Type::Nor:
			assert(gate.inputs().size() >= 2);
			break;
		case Gate::Type::Xor:
		case Gate::Type::Xnor:
			assert(gate.inputs().size() == 2);
			break;
		case Gate::Type::Not:
		case Gate::Type::Buff:
			assert(gate.inputs().size() == 1);
			break;
		default:
			assert(false);
	}

	return &gate;
}

Line* CircuitGraph::get_line(const std::string& name)
{
	auto it = m_name_to_line.find(name);

	if (it != m_name_to_line.end()) {
		return it->second;
	}

	return nullptr;
}

const Line* CircuitGraph::get_line(const std::string& name) const
{
	auto it = m_name_to_line.find(name);

	if (it != m_name_to_line.end()) {
		return it->second;
	}

	return nullptr;
}

const std::vector<Line*>& CircuitGraph::get_inputs() const
{
	return m_inputs;
}

const std::vector<Line*>& CircuitGraph::get_outputs() const
{
	return m_outputs;
}

const std::deque<Gate>& CircuitGraph::get_gates() const
{
	return m_gates;
}

const std::deque<Line>& CircuitGraph::get_lines() const
{
	return m_lines;
}

std::string CircuitGraph::get_graph_stats() const
{
	std::stringstream ss;

	ss << "# " << m_inputs.size() << " input" << (m_inputs.size() > 1 ? "s" : "") << "\n";
	ss << "# " << m_outputs.size() << " output" << (m_outputs.size() > 1 ? "s" : "") << "\n";
	ss << "# " << m_lines.size() << " line" << (m_lines.size() > 1 ? "s" : "") << "\n";
	ss << "# " << m_gates.size() << " gate" << (m_gates.size() > 1 ? "s" : "") << ":\n";

	std::map<Gate::Type, size_t> gate_types;
	for (const auto& gate : m_gates) {
		++gate_types[gate.get_type()];
	}

	for (const auto& type_count_pair : gate_types) {
		ss << "#     ";
		ss << type_count_pair.second << " ";
		ss << make_gate_name(type_count_pair.first);
		ss << "\n";
	}
	return ss.str();
}

Line* CircuitGraph::ensure_line(const std::string& name)
{
	auto it = m_name_to_line.find(name);

	if (it != m_name_to_line.end()) {
		return it->second;
	}

	m_lines.emplace_back(line_make_id());
	Line& line = m_lines.back();

	line.name = name;

	m_name_to_line[name] = &line;

	it = m_name_to_line.find(name);

	return &line;
}

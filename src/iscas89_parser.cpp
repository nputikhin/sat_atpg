#include "iscas89_parser.h"

#include "util/log.h"

#include <algorithm>
#include <string>
#include <regex>

bool Iscas89Parser::match_input(CircuitGraph& graph, const std::string& line)
{
	static const std::regex input_regex(R"r(\s*input\s*\(\s*(\S+)\s*\)\s*\r?\n?)r", std::regex_constants::icase);
	std::smatch matches;
	std::regex_match(line, matches, input_regex);
	if (matches.size() != 2)
		return false;

	graph.add_input(matches[1]);
	return true;
}

bool Iscas89Parser::match_output(CircuitGraph& graph, const std::string& line)
{
	static const std::regex output_regex(R"r(\s*output\s*\(\s*(\S+)\s*\)\s*\r?\n?)r", std::regex_constants::icase);
	std::smatch matches;
	std::regex_match(line, matches, output_regex);
	if (matches.size() != 2)
		return false;

	graph.add_output(matches[1]);
	return true;
}

bool Iscas89Parser::match_gate(CircuitGraph& graph, const std::string& line)
{
	static const std::regex gate_regex(R"r(\s*(\S+)\s*=\s*(\w+)\s*\(\s*((?:\S+\s*,?\s*)+)\s*\)\s*\r?\n?)r");
	std::smatch matches;
	std::regex_match(line, matches, gate_regex);

	if (matches.size() != 4)
		return false;

	std::string output = matches[1];
	std::string type_str = matches[2];

	std::transform(type_str.begin(), type_str.end(), type_str.begin(), ::tolower);

	Gate::Type gate_type = Gate::Type::Undefined;

	bool dff_gate = false;
	if (type_str == "and")
		gate_type = Gate::Type::And;
	else if (type_str == "nand")
		gate_type = Gate::Type::Nand;
	else if (type_str == "not")
		gate_type = Gate::Type::Not;
	else if (type_str == "or")
		gate_type = Gate::Type::Or;
	else if (type_str == "nor")
		gate_type = Gate::Type::Nor;
	else if (type_str == "xor")
		gate_type = Gate::Type::Xor;
	else if (type_str == "xnor")
		gate_type = Gate::Type::Xnor;
	else if (type_str == "buff" || type_str == "buf")
		gate_type = Gate::Type::Buff;
	else if (type_str == "dff") // Special case since we only work on combinational circuits - investigate further if something breaks
		dff_gate = true;

	if (!dff_gate && gate_type == Gate::Type::Undefined)
		return false;

	std::string gate_inputs = matches[3];

	std::vector<std::string> inputs;
	std::stringstream inputs_ss(gate_inputs);
	for (std::string input; std::getline(inputs_ss, input, ',');) {
		std::smatch input_matches;
		static const std::regex gate_input_regex(R"r(\s*(\S+)\s*)r");
		std::regex_match(input, input_matches, gate_input_regex);

		if (input_matches.size() != 2)
			return false;

		inputs.push_back(input_matches[1]);
	}

	if (dff_gate) {
		assert(inputs.size() == 1);
		graph.add_input(output);
		graph.add_output(inputs.front());
		return true;
	} else {
		graph.add_gate(gate_type, inputs, output);
		return true;
	}
}

bool Iscas89Parser::parse(std::istream& is, CircuitGraph& graph)
{
	static const std::regex comment_regex(R"r(\s*#.*\r?)r");
	static const std::regex empty_regex(R"r(\s+)r");
	size_t line_ctr = 0;
	for (std::string line; std::getline(is, line, '\n');) {
		++line_ctr;

		if (line.empty())
			continue;
		if (std::regex_match(line, empty_regex))
			continue;
		if (std::regex_match(line, comment_regex))
			continue;

		if (match_input(graph, line))
			continue;
		if (match_output(graph, line))
			continue;
		if (match_gate(graph, line))
			continue;

		log_error() << "Invalid line" << line_ctr << ": \"" << line << "\"";
		return false;
	}
	return true;
}

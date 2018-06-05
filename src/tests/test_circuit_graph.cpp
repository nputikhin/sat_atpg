#include "../circuit_graph.h"
#include "../iscas89_parser.h"

#include "circuits.h"

#include <catch.hpp>

#include <sstream>

TEST_CASE("empty circuit")
{
	CircuitGraph graph;
	REQUIRE(graph.get_inputs().empty());
	REQUIRE(graph.get_outputs().empty());

	REQUIRE(graph.get_line("input") == nullptr);
}

TEST_CASE("graph creation")
{
	CircuitGraph graph;
	Line* input = graph.add_input("input");
	Line* output = graph.add_output("output");

	REQUIRE(graph.get_inputs().size() == 1);
	REQUIRE(graph.get_outputs().size() == 1);

	Line* p_input = graph.get_line("input");
	REQUIRE(p_input);
	REQUIRE(p_input == input);
	REQUIRE_FALSE(p_input->is_output);
	REQUIRE(p_input->name == "input");

	Line* p_output = graph.get_line("output");
	REQUIRE(p_output);
	REQUIRE(output == output);
	REQUIRE(p_output->is_output);
	REQUIRE(p_output->name == "output");

	REQUIRE(p_input->id != p_output->id);

	SECTION("add gate") {
		Gate* gate = graph.add_gate(Gate::Type::Not, {"input"}, "output");
		REQUIRE(p_input->destinations.size() == 1);
		REQUIRE(p_input->destination_gates.size() == 1);

		Line::Connection connection = p_input->destinations.front();
		REQUIRE(connection.input_idx == 0);

		Gate* p_gate = connection.gate;

		REQUIRE(*p_input->destination_gates.begin() == p_gate);

		REQUIRE(p_gate == gate);
		REQUIRE(p_gate->get_type() == Gate::Type::Not);
		REQUIRE(p_gate->get_inputs().size() == 1);
		REQUIRE(p_gate->get_inputs().front() == p_input);
		REQUIRE(p_gate->get_output() == p_output);

		REQUIRE(p_output->source == p_gate);
	}
}

void test_gate(Gate* g, Gate::Type type, std::vector<Line*> inputs, Line* output)
{
	REQUIRE(g->get_type() == type);
	REQUIRE(g->get_output() == output);
	REQUIRE(g->get_inputs() == inputs);
	REQUIRE(output->source == g);
	for (Line* l : inputs) {
		REQUIRE(l->destination_gates.count(g));
	}
}

TEST_CASE("iscas85 c17 circuit")
{
	C17Circuit c17;

	test_gate(c17.g10, Gate::Type::Nand, {c17.l1, c17.l3},   c17.l10);
	test_gate(c17.g11, Gate::Type::Nand, {c17.l3, c17.l6},   c17.l11);
	test_gate(c17.g16, Gate::Type::Nand, {c17.l2, c17.l11},  c17.l16);
	test_gate(c17.g19, Gate::Type::Nand, {c17.l11, c17.l7},  c17.l19);
	test_gate(c17.g22, Gate::Type::Nand, {c17.l10, c17.l16}, c17.l22);
	test_gate(c17.g23, Gate::Type::Nand, {c17.l16, c17.l19}, c17.l23);
}

TEST_CASE("breadth first walking")
{
	C17Circuit c17;

	std::vector<const Gate*> walk_order;

	auto add_to_walk = [&walk_order](const Gate* gate) {
		walk_order.push_back(gate);
	};

	SECTION("towards output from single gate") {
		walk_gates_breadth_first({c17.g11}, add_to_walk, true);

		REQUIRE(walk_order == std::vector<const Gate*>{c17.g11, c17.g16, c17.g19, c17.g22, c17.g23});
	}

	SECTION("towards output from multiple gates") {
		walk_gates_breadth_first({c17.g10, c17.g11}, add_to_walk, true);

		REQUIRE(walk_order == std::vector<const Gate*>{c17.g10, c17.g11, c17.g22, c17.g16, c17.g19, c17.g23});
	}

	SECTION("towards input from single gate") {
		walk_gates_breadth_first({c17.g23}, add_to_walk, false);

		REQUIRE(walk_order == std::vector<const Gate*>{c17.g23, c17.g16, c17.g19, c17.g11});
	}

	SECTION("towards input from multiple gates") {
		walk_gates_breadth_first({c17.g23, c17.g22}, add_to_walk, false);

		REQUIRE(walk_order == std::vector<const Gate*>{c17.g23, c17.g22, c17.g16, c17.g19, c17.g10, c17.g11});
	}
}

TEST_CASE("gate expansion") {
	CircuitGraph graph;
	std::array<Line*, 4> inputs;
	for (size_t i = 0; i < inputs.size(); ++i) {
		graph.add_input(std::to_string(i));
		inputs[i] = graph.get_line(std::to_string(i));
	}
	graph.add_output("out");
	Line* output = graph.get_line("out");

	SECTION("non-expandable gates") {
		std::vector<std::pair<Gate::Type, size_t>> non_expandable_gates = {
			{Gate::Type::Buff, 1},
			{Gate::Type::Not,  1},
			{Gate::Type::And,  2},
			{Gate::Type::Nand, 2},
			{Gate::Type::Or,   2},
			{Gate::Type::Nor,  2},
			{Gate::Type::Xor,  2},
			{Gate::Type::Xnor, 2},
		};
		for (auto type_size_pair : non_expandable_gates) {
			SECTION(std::string("non-expandable gate ") + std::to_string((int)type_size_pair.first)) {
				std::vector<std::string> input_names;
				for (size_t i = 0; i < type_size_pair.second; ++i) {
					input_names.push_back(inputs[i]->name);
				}
				graph.add_gate(type_size_pair.first, input_names, output->name);
				const Gate& gate = graph.get_gates().front();
				REQUIRE(gate.get_expanded().size() == 1);
			}
		}
	}

	SECTION("expandable gates") {
		std::vector<std::string> input_names;
		for (size_t i = 0; i < inputs.size(); ++i) {
			input_names.push_back(inputs[i]->name);
		}
		std::vector<std::pair<Gate::Type, Gate::Type>> expandable_gates = {
			{Gate::Type::And,  Gate::Type::And},
			{Gate::Type::Nand, Gate::Type::And},
			{Gate::Type::Or,   Gate::Type::Or},
			{Gate::Type::Nor,  Gate::Type::Or},
		};

		for (auto top_gate_other_gates_pair : expandable_gates) {
			SECTION(std::string("expandable gate ") + std::to_string((int)top_gate_other_gates_pair.first)) {
				Gate::Type top_gate = top_gate_other_gates_pair.first;
				Gate::Type other_gates = top_gate_other_gates_pair.second;
				graph.add_gate(top_gate, input_names, output->name);
				const Gate& gate = graph.get_gates().front();
				auto& expanded = gate.get_expanded();
				REQUIRE(expanded.size() == inputs.size() - 1);

				const Gate& eg1 = *expanded.at(0);
				REQUIRE(eg1.get_type() == other_gates);
				REQUIRE(eg1.get_inputs() == std::vector<Line*>{inputs[2], inputs[3]});

				Line* eg1_out = eg1.get_output();
				const Gate& eg2 = *expanded.at(1);
				REQUIRE(eg2.get_type() == other_gates);
				REQUIRE(eg2.get_inputs() == std::vector<Line*>{inputs[1], eg1_out});

				Line* eg2_out = eg2.get_output();
				const Gate& eg3 = *expanded.at(2);
				REQUIRE(eg3.get_type() == top_gate);
				REQUIRE(eg3.get_inputs() == std::vector<Line*>{inputs[0], eg2_out});
				REQUIRE(eg3.get_output() == output);
			}
		}
	}
}

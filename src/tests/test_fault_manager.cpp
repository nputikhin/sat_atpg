#include "../fault_manager.h"

#include "circuits.h"

#include <catch.hpp>

namespace Catch {
	template<>
	struct StringMaker<Fault> {
		static std::string convert(const Fault& value ) {
			std::stringstream ss;
			ss << "\n[";
			ss << value.line->name << " ";
			if (value.is_stem) {
				ss << "S";
			} else if (value.is_primary_output) {
				ss << "PO";
			} else {
				ss << "-> " << value.connection.gate->get_output()->name;
				ss << "/I" << value.connection.input_idx;
			}
			ss << " @" << (int)value.stuck_at;
			ss << "]";
			return ss.str();
		}
	};
}

std::vector<Fault> extract_faults(const CircuitGraph& graph)
{
	FaultManager manager(graph);
	std::vector<Fault> faults;
	while (manager.has_faults_left()) {
		faults.push_back(manager.next_fault());
	}
	return faults;
}

TEST_CASE("nan xor circuit faults")
{
	NandXorCircuit nxc;
	auto faults = extract_faults(nxc.graph);
	std::vector<Fault> expected_faults = {
		{nxc.l1, 0, true},
		{nxc.l1, 1, true},
		{nxc.l1, 1, false, nxc.g4, 0},
		{nxc.l1, 1, false, nxc.g5, 0},

		{nxc.l2, 0, true},
		{nxc.l2, 1, true},
		{nxc.l2, 1, false, nxc.g4, 1},
		{nxc.l2, 1, false, nxc.g6, 0},

		{nxc.l3, 0, true},
		{nxc.l3, 1, true},

		{nxc.l4, 0, true},
		{nxc.l4, 1, true},
		{nxc.l4, 1, false, nxc.g5, 1},
		{nxc.l4, 1, false, nxc.g6, 1},

		{nxc.l5, 1, true, nxc.g3, 0},
		{nxc.l6, 1, true, nxc.g3, 1},
	};
	REQUIRE_THAT(faults, Catch::Matchers::UnorderedEquals(expected_faults));
}

TEST_CASE("c17 faults")
{
	C17Circuit c17;
	FaultManager manager(c17.graph);
	std::vector<Fault> faults;
	while (manager.has_faults_left()) {
		faults.push_back(manager.next_fault());
	}

	REQUIRE(faults.size() == 22);
}

TEST_CASE("s27 faults")
{
	S27Circuit s27;
	auto faults = extract_faults(s27.graph);

	std::vector<Fault> expected_faults = {
		{s27.l1, 0, false, s27.g12},
		{s27.l2, 0, false, s27.g13},
		{s27.l3, 0, false, s27.g16},
		{s27.l5, 0, false, s27.g11},
		{s27.l6, 1, false, s27.g8},
		{s27.l7, 0, false, s27.g12},
		{s27.l10, 0, true},
		{s27.l10, 1, true},
		{s27.l13, 0, true},
		{s27.l13, 1, true},
		{s27.l14, 0, true},
		{s27.l14, 1, true},
		{s27.l14, 1, false, s27.g8},
		{s27.l14, 0, false, s27.g10},
		{s27.l11, 0, true},
		{s27.l11, 1, true},
		{s27.l11, 0, false, s27.g10},
		{s27.l8, 0, true},
		{s27.l8, 1, true},
		{s27.l8, 0, false, s27.g15},
		{s27.l8, 0, false, s27.g16},
		{s27.l12, 0, true},
		{s27.l12, 1, true},
		{s27.l12, 0, false, s27.g15},
		{s27.l12, 0, false, s27.g13},
		{s27.l15, 1, true, s27.g9},
		{s27.l16, 1, true, s27.g9},
		{s27.l9, 0, true, s27.g11},
		{s27.l11_extra, 0, true},
		{s27.l11_extra, 1, true},
		{s27.l17, 0, true},
		{s27.l17, 1, true},
	};
	REQUIRE_THAT(faults, Catch::Matchers::UnorderedEquals(expected_faults));
}

TEST_CASE("single gate faults")
{
	CircuitGraph graph;
	Line* inp1 = graph.add_input("i1");
	Line* outp = graph.add_output("o");

	SECTION("two input") {
		Line* inp2 = graph.add_input("i2");
		std::vector<std::pair<Gate::Type, size_t>> gates_sizes =
		{
			{Gate::Type::And,  4},
			{Gate::Type::Nand, 4},
			{Gate::Type::Or,   4},
			{Gate::Type::Nor,  4},
			{Gate::Type::Xor,  6},
			{Gate::Type::Xnor, 6},
		};
		for (auto gate_size_pair : gates_sizes) {
			Gate::Type gate_type = gate_size_pair.first;
			size_t fault_size = gate_size_pair.second;
			SECTION(std::to_string((int)gate_type)) {
				graph.add_gate(gate_type,{inp1->name, inp2->name}, outp->name);

				auto faults = extract_faults(graph);
				CAPTURE(faults);
				REQUIRE(faults.size() == fault_size);
			}
		}
	}

	SECTION("buff") {
		graph.add_gate(Gate::Type::Buff,{inp1->name}, outp->name);

		auto faults = extract_faults(graph);
		CAPTURE(faults);
		REQUIRE(faults.size() == 2);
	}
	SECTION("not") {
		graph.add_gate(Gate::Type::Not,{inp1->name}, outp->name);

		auto faults = extract_faults(graph);
		CAPTURE(faults);
		REQUIRE(faults.size() == 2);
	}
}

TEST_CASE("buff or not chain") {
	CircuitGraph graph;
	Line* inp1 = graph.add_input("i1");
	Line* outp = graph.add_output("o");
	graph.add_gate(Gate::Type::Not, {inp1->name}, "g");
	graph.add_gate(Gate::Type::Buff, {"g"}, outp->name);

	auto faults = extract_faults(graph);
	CAPTURE(faults);
	REQUIRE(faults.size() == 2);
}

TEST_CASE("fanout from not") {
	CircuitGraph graph;
	graph.add_input("a");
	graph.add_input("b");
	graph.add_input("c");
	graph.add_output("y");
	graph.add_output("z");

	graph.add_gate(Gate::Type::Not, {"a"}, "g1");
	graph.add_gate(Gate::Type::And, {"g1", "b"}, "y");
	graph.add_gate(Gate::Type::Nor, {"g1", "c"}, "z");

	auto faults = extract_faults(graph);
	CAPTURE(faults);
	REQUIRE(faults.size() == 10);
}

TEST_CASE("output is used as gate input") {
	CircuitGraph graph;
	graph.add_input("a");
	graph.add_input("b");
	graph.add_output("y");
	graph.add_output("z");

	graph.add_gate(Gate::Type::Nor, {"a", "b"}, "y");
	graph.add_gate(Gate::Type::Buff, {"y"}, "z");

	auto faults = extract_faults(graph);
	CAPTURE(faults);
	REQUIRE(faults.size() == 8);
}

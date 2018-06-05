#include <catch.hpp>

#include "circuits.h"
#include "../fault_cnf.h"
#include "../fault_manager.h"
#include "../util/log.h"

#include "../sat/sat_solver.h"

#include <type_traits>

namespace Catch {
	template<>
	struct StringMaker<const Line*> {
		static std::string convert(const Line* const& value) {
			if (!value)
				return "nullptr";
			std::stringstream ss;
			ss << "[ l: " << value->name << "]";
			return ss.str();
		}
	};
}

TEST_CASE("c17 11/O S-A-1 fanout cone") {
	C17Circuit c17;

	Fault fault;
	fault.line = c17.l11;
	fault.is_stem = true;
	fault.stuck_at = 1;

	FanoutConeInfo fanout_cone = make_fanout_cone(fault);

	std::set<const Line*> boundary_lines = {c17.l2, c17.l7, c17.l10};
	std::set<const Line*> lines_inside = {c17.l11, c17.l16, c17.l19, c17.l22, c17.l23};
	std::set<const Line*> primary_outputs_inside = {c17.l22, c17.l23};

	REQUIRE(fanout_cone.boundary_lines == boundary_lines);
	REQUIRE(fanout_cone.lines_inside == lines_inside);
	REQUIRE(fanout_cone.primary_outputs_inside == primary_outputs_inside);
}

TEST_CASE("c17 3 S-A-1 fanout cone") {
	C17Circuit c17;

	Fault fault;
	fault.line = c17.l3;
	fault.is_stem = true;
	fault.stuck_at = 1;

	FanoutConeInfo fanout_cone = make_fanout_cone(fault);

	std::set<const Line*> boundary_lines = {c17.l1, c17.l6, c17.l2, c17.l7};
	std::set<const Line*> lines_inside = {c17.l3, c17.l10, c17.l11, c17.l16, c17.l19, c17.l22, c17.l23};
	std::set<const Line*> primary_outputs_inside = {c17.l22, c17.l23};

	REQUIRE(fanout_cone.boundary_lines == boundary_lines);
	REQUIRE(fanout_cone.lines_inside == lines_inside);
	REQUIRE(fanout_cone.primary_outputs_inside == primary_outputs_inside);
}

TEST_CASE("c17 16/I2 S-A-1 fanout cone") {
	C17Circuit c17;

	Fault fault(c17.g16->get_inputs().at(1), 1, false, c17.g16, 1);

	FanoutConeInfo fanout_cone = make_fanout_cone(fault);

	std::set<const Line*> boundary_lines = {c17.l2, c17.l10, c17.l19};
	std::set<const Line*> lines_inside = {c17.l16, c17.l11, c17.l22, c17.l23};
	std::set<const Line*> primary_outputs_inside = {c17.l22, c17.l23};

	REQUIRE(fanout_cone.boundary_lines == boundary_lines);
	REQUIRE(fanout_cone.lines_inside == lines_inside);
	REQUIRE(fanout_cone.primary_outputs_inside == primary_outputs_inside);
}

TEST_CASE("fanout cone for non-stem fault treats other inputs as boundaries") {
	TestCircuit tc;

	Fault fault(tc.x1, 0, false, tc.g->source, 0);

	FanoutConeInfo fanout_cone = make_fanout_cone(fault);

	std::set<const Line*> boundary_lines = {tc.x2, tc.h};
	std::set<const Line*> lines_inside = {tc.x1, tc.g, tc.y};
	std::set<const Line*> primary_outputs_inside = {tc.y};

	REQUIRE(fanout_cone.boundary_lines == boundary_lines);
	REQUIRE(fanout_cone.lines_inside == lines_inside);
	REQUIRE(fanout_cone.primary_outputs_inside == primary_outputs_inside);
}

bool is_detectable(const Fault& f, FaultCnfMaker& maker, SatSolver& solver)
{
	Cnf c;
	maker.make_fault(f, c);
	return (solver.solve(c) == SatSolver::SolveStatus::Sat);
}

bool is_detectable(const Fault& f, const CircuitGraph& graph)
{
	auto solver = SolverFactory::make_solver();
	FaultCnfMaker maker(graph);
	return is_detectable(f, maker, *solver);
}

bool no_solver()
{
	if (!SolverFactory::make_solver()) {
		FAIL("This test needs SAT solver to run");
		return true;
	}
	return false;
}

TEST_CASE("c17 all faults") {
	if (no_solver()) return;

	C17Circuit c17;
	FaultManager mgr(c17.graph);
	FaultCnfMaker fault_maker(c17.graph);
	auto solver = SolverFactory::make_solver();

	while (mgr.has_faults_left()) {
		Fault f = mgr.next_fault();
		REQUIRE(is_detectable(f, fault_maker, *solver));
	}
}

TEST_CASE("check all test circuit faults") {
	if (no_solver()) return;

	TestCircuit tc;

	auto solver = SolverFactory::make_solver();
	FaultCnfMaker maker(tc.graph);

	std::vector<std::pair<Fault, std::vector<uint32_t>>> fault_test_vectors = {
		{Fault(tc.x1, 0, false, tc.g->source), {110, 111}},
		{Fault(tc.x1, 1, false, tc.g->source), {10, 11}},
		{Fault(tc.x2, 0, true), {11, 110}},
		{Fault(tc.x2, 1, true), {1, 100}},
		{Fault(tc.x3, 0, false, tc.h->source), {1, 101}},
		{Fault(tc.x3, 1, false, tc.h->source), {0, 100}},
		{Fault(tc.x2, 0, false, tc.g->source), {110, 111}},
		{Fault(tc.x2, 1, false, tc.g->source), {100}},
		{Fault(tc.x2, 0, false, tc.f->source), {11}},
		{Fault(tc.x2, 1, false, tc.f->source), {1, 101}},
		{Fault(tc.f, 0, true), {1, 101}},
		{Fault(tc.f, 1, true), {11}},
		{Fault(tc.g, 0, true), {110, 111}},
		{Fault(tc.g, 1, true), {0, 10, 11, 100}},
		{Fault(tc.h, 0, true), {1, 101}},
		{Fault(tc.h, 1, true), {0, 10, 11, 100}},
		{Fault(tc.y, 0, true), {1, 101, 110, 111}},
		{Fault(tc.y, 1, true), {0, 10, 11, 100}},
	};

	std::vector<Line*> inputs = {tc.x3, tc.x2, tc.x1};

	for (auto& fault_test_vectors_pair : fault_test_vectors) {
		const Fault& fault = fault_test_vectors_pair.first;
		auto& test_vectors = fault_test_vectors_pair.second;

		REQUIRE(is_detectable(fault, maker, *solver));

		uint32_t input_vector = 0;
		uint32_t mul = 10;
		for (size_t i = 0; i < inputs.size(); ++i) {
			Line* l = inputs.at(i);
			int8_t val = solver->get_value(line_to_literal(l->id));
			assert(val);
			val = val <= 0 ? 0 : 1;
			input_vector |= val * (mul/10);
			mul *= 10;
		}

		CAPTURE(fault.line);
		CAPTURE(fault.stuck_at);

		const Line* fault_dest = fault.connection.gate ? fault.connection.gate->get_output() : nullptr;
		CAPTURE(fault_dest);

		REQUIRE_THAT(test_vectors, Catch::Matchers::VectorContains(input_vector));
	}
}

void require_fault_count(const CircuitGraph& graph, size_t total, size_t detectable) {
	FaultManager mgr(graph);
	FaultCnfMaker maker(graph);
	auto solver = SolverFactory::make_solver();

	size_t detectable_actual = 0;
	size_t total_actual = 0;

	while (mgr.has_faults_left()) {
		Fault f = mgr.next_fault();
		++total_actual;
		if (is_detectable(f, maker, *solver))
			++detectable_actual;
	}

	REQUIRE(total_actual == total);
	REQUIRE(detectable_actual == detectable);
}

TEST_CASE("test circuit with expandable gates") {
	if (no_solver()) return;

	TestCircuitWithExpandableGates tc;
	auto solver = SolverFactory::make_solver();
	FaultCnfMaker maker(tc.graph);

	SECTION("g16/O S-A-1 undetectable") {
		Fault fault(tc.g16, 1, true);
		REQUIRE(!is_detectable(fault, maker, *solver));
	}

	SECTION("g17/I1 S-A-0 undetectable") {
		Fault fault(tc.c, 0, false, tc.g17->source, 0);
		REQUIRE(!is_detectable(fault, maker, *solver));
	}

	SECTION("fault count") {
		require_fault_count(tc.graph, 41, 37);
	}
}

TEST_CASE("non-stem fault on input that also goes to other gate") {
	if (no_solver()) return;

	// g2 I1 S-A-1
	//
	// INPUT(a)
	// INPUT(b)
	//
	// OUTPUT(z)
	//
	// g1 = NOT(a)
	// g2 = NAND(a, b)
	//
	// DETECTABLE:
	// z = NAND(g1, g2)
	//
	// UNDETECTABLE & all faults:
	// z = NAND(a, g1, g2)

	CircuitGraph graph;

	Line* a = graph.add_input("a");
	graph.add_input("b");
	graph.add_output("z");

	graph.add_gate(Gate::Type::Not, {"a"}, "g1");
	Gate* g2 = graph.add_gate(Gate::Type::Nand, {"a", "b"}, "g2");

	Fault fault(a, 1, false, g2, 0);

	SECTION("detectable fault") {
		graph.add_gate(Gate::Type::Nand, {"g1", "g2"}, "z");
		REQUIRE(is_detectable(fault, graph));
	}

	SECTION("undetectable fault") {
		graph.add_gate(Gate::Type::Nand, {"a", "g1", "g2"}, "z");
		REQUIRE(!is_detectable(fault, graph));
	}

	SECTION("fault count") {
		graph.add_gate(Gate::Type::Nand, {"a", "g1", "g2"}, "z");
		require_fault_count(graph, 9, 3);
	}
}

TEST_CASE("duplicate input to gate") {
	if (no_solver()) return;

	CircuitGraph graph;

	Line* x = graph.add_input("x");
	Line* y = graph.add_output("y");

	graph.add_gate(Gate::Type::And, {"x", "x"}, "y");

	SECTION("detectable fault") {
		Fault fault(x, 1, true);
		REQUIRE(is_detectable(fault, graph));
	}

	SECTION("undetectable fault") {
		Fault fault(x, 0, false, y->source);
		REQUIRE(!is_detectable(fault, graph));
	}
}

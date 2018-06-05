#include <catch.hpp>

#include "../cnf.h"
#include "../circuit_graph.h"
#include "../circuit_to_cnf.h"

#include "../util/log.h"

TEST_CASE("cnf")
{
	Cnf cnf;
	REQUIRE(cnf.get_clauses().empty());

	clause_t c1 = {1, 2, -3};
	clause_t c2 = {-1, 3};
	clause_t c3 = {-2, 3};
	cnf.add_clause(c1);
	cnf.add_clause(c2);
	cnf.add_clause(c3);

	const auto& clauses = cnf.get_clauses();
	REQUIRE(clauses.size() == 3);
	REQUIRE(clauses[0] == c1);
	REQUIRE(clauses[1] == c2);
	REQUIRE(clauses[2] == c3);

	SECTION("test assignments") {
		REQUIRE_FALSE(cnf.is_satisfied({0, 0, 0, 1}));
		REQUIRE(cnf.is_satisfied({0, 0, 1, 1}));
	}
}

assignment_t make_assignment(size_t size, size_t true_vars)
{
	assignment_t result(size, 0);

	std::fill(result.rbegin(), result.rbegin() + true_vars, 1);
	return result;
}

void require_output(bool val, size_t output_line, assignment_t& assignment, const Cnf& cnf)
{
	INFO(cnf.get_dimacs_str());
	CAPTURE(val);
	assignment[line_to_literal(output_line)] = val;
	CAPTURE(assignment);
	REQUIRE(cnf.is_satisfied(assignment));
	assignment[line_to_literal(output_line)] = !val;
	CAPTURE(assignment);
	REQUIRE_FALSE(cnf.is_satisfied(assignment));
}

TEST_CASE("gates to cnf")
{
	CircuitToCnfTransformer transfromer;

	const size_t max_inputs = 5;

	for (size_t num_inputs = 1; num_inputs <= max_inputs; ++num_inputs) {
		CircuitGraph graph;

		std::vector<std::string> inputs;
		for (size_t i = 1; i <= num_inputs; ++i) {
			graph.add_input(std::to_string(i));
			REQUIRE(graph.get_line(std::to_string(i)));
			inputs.push_back(std::to_string(i));
		}

		graph.add_output("out");
		Line* output = graph.get_line("out");
		REQUIRE(output);

		for (size_t num_true_inputs = 0; num_true_inputs <= num_inputs; ++num_true_inputs) {
			assignment_t input_assignment = make_assignment(num_inputs, num_true_inputs);

			bool at_least_one_input_is_true = num_true_inputs > 0;
			bool at_least_one_input_is_false = num_true_inputs < num_inputs;
			bool all_inputs_are_true = !at_least_one_input_is_false;
			bool all_inputs_are_false = !at_least_one_input_is_true;

			(void)all_inputs_are_false;

			SECTION (std::to_string(num_true_inputs) + " true out of " + std::to_string(num_inputs)) {
				do {
					std::string perm_string = "(";
					for (auto v : input_assignment) {
						perm_string += std::to_string(v);
					}
					perm_string += ") gate ";

					assignment_t assignment;

					assignment.push_back(0); // literals start at 1

					assignment.insert(assignment.end(), input_assignment.begin(), input_assignment.end());

					assignment.insert(assignment.begin() + line_to_literal(output->id), 0);

					if (num_inputs > 1) {
						SECTION(perm_string + "and") {
							graph.add_gate(Gate::Type::And, inputs, "out");
							Cnf cnf = transfromer.make_cnf(graph);
							if (at_least_one_input_is_false) {
								require_output(false, output->id, assignment, cnf);
							}

							if (all_inputs_are_true) {
								require_output(true, output->id, assignment, cnf);
							}
						}

						SECTION(perm_string + "nand") {
							graph.add_gate(Gate::Type::Nand, inputs, "out");
							Cnf cnf = transfromer.make_cnf(graph);

							if (at_least_one_input_is_false) {
								require_output(true, output->id, assignment, cnf);
							}

							if (all_inputs_are_true) {
								require_output(false, output->id, assignment, cnf);
							}
						}

						SECTION(perm_string + "or") {
							graph.add_gate(Gate::Type::Or, inputs, "out");
							Cnf cnf = transfromer.make_cnf(graph);
							if (at_least_one_input_is_true) {
								require_output(true, output->id, assignment, cnf);
							}

							if (all_inputs_are_false) {
								require_output(false, output->id, assignment, cnf);
							}
						}

						SECTION(perm_string + "nor") {
							graph.add_gate(Gate::Type::Nor, inputs, "out");
							Cnf cnf = transfromer.make_cnf(graph);
							if (at_least_one_input_is_true) {
								require_output(false, output->id, assignment, cnf);
							}

							if (all_inputs_are_false) {
								require_output(true, output->id, assignment, cnf);
							}
						}
					}

					if (num_inputs == 1) {
						SECTION(perm_string + "not") {
							graph.add_gate(Gate::Type::Not, inputs, "out");
							Cnf cnf = transfromer.make_cnf(graph);
							if (all_inputs_are_true) {
								require_output(false, output->id, assignment, cnf);
							}

							if (all_inputs_are_false) {
								require_output(true, output->id, assignment, cnf);
							}
						}

						SECTION(perm_string + "buff") {
							graph.add_gate(Gate::Type::Buff, inputs, "out");
							Cnf cnf = transfromer.make_cnf(graph);
							if (all_inputs_are_true) {
								require_output(true, output->id, assignment, cnf);
							}

							if (all_inputs_are_false) {
								require_output(false, output->id, assignment, cnf);
							}
						}
					}

					if (num_inputs == 2) {
						SECTION(perm_string + "xor") {
							graph.add_gate(Gate::Type::Xor, inputs, "out");
							Cnf cnf = transfromer.make_cnf(graph);
							if (at_least_one_input_is_true && !all_inputs_are_true) {
								require_output(true, output->id, assignment, cnf);
							} else {
								require_output(false, output->id, assignment, cnf);
							}
						}

						SECTION(perm_string + "xnor") {
							graph.add_gate(Gate::Type::Xnor, inputs, "out");
							Cnf cnf = transfromer.make_cnf(graph);
							if (at_least_one_input_is_true && !all_inputs_are_true) {
								require_output(false, output->id, assignment, cnf);
							} else {
								require_output(true, output->id, assignment, cnf);
							}
						}
					}

				} while (std::next_permutation(input_assignment.begin(), input_assignment.end()));
			}
		}
	}
}


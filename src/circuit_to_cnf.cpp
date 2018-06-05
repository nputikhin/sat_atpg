#include "circuit_to_cnf.h"

#include "util/log.h"

#include <cassert>

literal_t line_to_literal(size_t id)
{
	return id + 1;
}

size_t literal_to_line(literal_t l)
{
	return std::abs(l) - 1;
}

// Adds clauses with signs dependent on template params:
// OUT v IN_1
// OUT v IN_2
// ...
// OUT v IN_n
// OUT v IN_1 v IN_2 v ... v IN_n
template<int sign_in_expr, int sign_in_final, int sign_out_expr, int sign_out_final>
std::vector<clause_t> make_standard_gate(const std::vector<Line*>& inputs, const Line* output)
{
	literal_t output_literal = line_to_literal(output->id);
	clause_t final_clause;
	final_clause.reserve(inputs.size() + 1);

	final_clause.push_back(output_literal * sign_out_final);

	std::vector<clause_t> result;
	result.reserve(inputs.size() + 1);

	for (const Line* input : inputs) {
		literal_t input_literal = line_to_literal(input->id);
		result.push_back({output_literal * sign_out_expr, input_literal * sign_in_expr});
		final_clause.push_back(input_literal * sign_in_final);
	}

	result.push_back(final_clause);
	return result;
}

// Adds clauses for xor (sign_neq_out=1) or xnor (sing_neq_out=-1):
// -IN_1 v -IN_2 v -OUT
//  IN_1 v  IN_2 v -OUT
//  IN_1 v -IN_2 v  OUT
// -IN_1 v  IN_2 v  OUT
template<int sign_neq_out>
std::vector<clause_t> make_xor_gate(const std::vector<Line*>& inputs, const Line* output)
{
	assert(inputs.size() == 2);
	literal_t li1 = line_to_literal(inputs[0]->id);
	literal_t li2 = line_to_literal(inputs[1]->id);
	literal_t lo  = line_to_literal(output->id);

	std::vector<clause_t> result;
	result.reserve(4);

	result.push_back({-li1, -li2, -sign_neq_out*lo});
	result.push_back({ li1,  li2, -sign_neq_out*lo});
	result.push_back({ li1, -li2,  sign_neq_out*lo});
	result.push_back({-li1,  li2,  sign_neq_out*lo});

	return result;
}

std::vector<clause_t> CircuitToCnfTransformer::make_clauses(const Gate& gate)
{
	switch (gate.get_type()) {
		case Gate::Type::Buff:
			assert(gate.get_inputs().size() == 1);
			//[[fallthrough]];
		case Gate::Type::And:
			return make_standard_gate<1, -1, -1, 1>(gate.get_inputs(), gate.get_output());
			break;
		case Gate::Type::Not:
			assert(gate.get_inputs().size() == 1);
			//[[fallthrough]];
		case Gate::Type::Nand:
			return make_standard_gate<1, -1, 1, -1>(gate.get_inputs(), gate.get_output());
			break;
		case Gate::Type::Or:
			return make_standard_gate<-1, 1, 1, -1>(gate.get_inputs(), gate.get_output());
			break;
		case Gate::Type::Nor:
			return make_standard_gate<-1, 1, -1, 1>(gate.get_inputs(), gate.get_output());
			break;
		case Gate::Type::Xor:
			return make_xor_gate<1>(gate.get_inputs(), gate.get_output());
			break;
		case Gate::Type::Xnor:
			return make_xor_gate<-1>(gate.get_inputs(), gate.get_output());
			break;
		default:
			log_error() << "Unsupported gate:" << (uint32_t)gate.get_type();
			assert(false);
	}
	assert(false);
	return {};
}

Cnf CircuitToCnfTransformer::make_cnf(const CircuitGraph& graph, bool expand_gates)
{
	Cnf cnf;
	for (const Gate& gate : graph.get_gates()) {
		if (!expand_gates) {
			auto gate_clauses = make_clauses(gate);
			cnf.add_clauses(gate_clauses);
		} else {
			for (const Gate* extended_gate : gate.get_expanded()) {
				auto gate_clauses = make_clauses(*extended_gate);
				cnf.add_clauses(gate_clauses);
			}
		}
	}
	return cnf;
}

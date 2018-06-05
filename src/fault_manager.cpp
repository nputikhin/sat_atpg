#include "fault_manager.h"

#include "util/log.h"

#include <algorithm>

FaultManager::FaultManager(const CircuitGraph& circuit)
{
	for (const Line& line : circuit.get_lines()) {
		// line needs two faults <=> line leads to output OR line gate has fanout >= 2
		// Otherwise gate needs output determined by line destination
		// 	XOR XNOR - 2 faults
		// 	AND NAND OR NOR - 1 fault
		// 	NOT BUFF - no faults

		bool has_fanout_branches = line.is_output ? !line.destinations.empty() : line.destinations.size() > 1;
		bool needs_stem_fault = has_fanout_branches || line.is_output;
		if (needs_stem_fault) {
			add_stem_fault(line);

			if (line.is_output && has_fanout_branches) {
				Fault fault;
				fault.is_stem = false;
				fault.is_primary_output = true;
				fault.line = &line;

				fault.stuck_at = 0;
				m_faults.push_back(fault);

				fault.stuck_at = 1;
				m_faults.push_back(fault);
			}

			for (const Line::Connection& connection : line.destinations) {
				if (connection.gate->get_type() == Gate::Type::Not || connection.gate->get_type() == Gate::Type::Buff) {
					continue;
				}

				add_gate_input_fault(line, connection, false);
			}
		} else {
			if (line.destinations.empty()) {
				log_error() << "Invalid line:" << line.name;
				std::exit(1);
			}
			assert(!line.destinations.empty());
			assert(line.destinations.size() == 1);
			const Line::Connection& connection = line.destinations.front();
			if (connection.gate->get_type() != Gate::Type::Not && connection.gate->get_type() != Gate::Type::Buff) {
				bool is_stem = line.source;
				add_gate_input_fault(line, connection, is_stem);
			}
		}

	}
	m_it = m_faults.begin();
}

bool FaultManager::has_faults_left()
{
	return m_it != m_faults.end();
}

Fault FaultManager::next_fault()
{
	return *m_it++;
}

void FaultManager::add_stem_fault(const Line& line)
{
	Fault fault;
	fault.is_stem = true;
	fault.line = &line;

	fault.stuck_at = 0;
	m_faults.push_back(fault);

	fault.stuck_at = 1;
	m_faults.push_back(fault);
}

void FaultManager::add_gate_input_fault(const Line& line, const Line::Connection& connection, bool is_stem)
{
	Gate* gate = connection.gate;
	assert(gate);
	assert(std::find(line.destinations.begin(), line.destinations.end(), connection) != line.destinations.end());

	Fault fault;
	fault.is_stem = is_stem;
	fault.line = &line;

	fault.connection = connection;

	if (gate->get_type() == Gate::Type::Xor || gate->get_type() == Gate::Type::Xnor) {
		fault.stuck_at = 1;
		m_faults.push_back(fault);

		fault.stuck_at = 0;
		m_faults.push_back(fault);
		return;
	}

	int8_t stuck_at = -1;

	switch(gate->get_type()) {
		case Gate::Type::Nand:
			//[[fallthrough]];
		case Gate::Type::And:
			stuck_at = 1;
			break;
		case Gate::Type::Or:
			//[[fallthrough]];
		case Gate::Type::Nor:
			stuck_at = 0;
			break;
		default:
			log_debug() << "wrong gate type:" << gate->get_type();
			assert(false);
			return;
	}

	assert(stuck_at != -1);

	fault.stuck_at = stuck_at;
	m_faults.push_back(fault);
}

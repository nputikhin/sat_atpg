#pragma once

#include <string>
#include <cstdint>
#include <limits>
#include <vector>
#include <deque>
#include <set>
#include <unordered_map>
#include <cassert>

#include "object_set.h"
#include "util/log.h"

class Gate;
class CircuitGraph;

struct Line
{
	Line(size_t id, bool is_generated = false)
		: is_generated(is_generated)
		, id(id)
	{}

	Line(const Line&) = delete;

	void connect_as_input(Gate* gate, size_t input_idx)
	{
		destinations.emplace_back();
		destinations.back().gate = gate;
		destinations.back().input_idx = input_idx;
		destination_gates.insert(gate);
	}

	bool has_multiple_outputs_to_gate() const
	{
		return destinations.size() != destination_gates.size();
	}

	Gate* source = nullptr; // nullptr means input port
	struct Connection
	{
		Gate* gate = nullptr;
		size_t input_idx = std::numeric_limits<size_t>::max();

		bool operator==(const Connection& other) const
		{
			return std::tie(gate, input_idx) == std::tie(other.gate, other.input_idx);
		}

		bool operator!=(const Connection& other) const
		{
			return !operator==(other);
		}
	};
	std::vector<Connection> destinations;
	std::set<Gate*> destination_gates;

	bool is_output = false;
	bool is_generated = false;

	std::string name;
	size_t id = 0;
};

class IdMaker
{
public:
	IdMaker() = default;
	IdMaker(const IdMaker&) = delete;

	size_t line_id_end() const { return m_line_id; }
	size_t line_make_id() { return m_line_id++; }

	size_t gate_id_end() const { return m_gate_id; }
	size_t gate_make_id() { return m_gate_id++; }

private:
	size_t m_gate_id = 0;
	size_t m_line_id = 0;
};

class Gate
{
public:
	enum class Type: uint32_t
	{
		And,
		Nand,
		Not,
		Or,
		Nor,
		Xor,
		Xnor,
		Buff,

		Undefined,
	};

	Gate(IdMaker& id_maker, Type type, Line* output, std::vector<Line*>&& inputs);

	Gate(const Gate&) = delete;

	Type get_type() const { return m_type; }
	Type& type() { return m_type; }

	const std::vector<Line*>& get_inputs() const { return m_inputs; }
	std::vector<Line*>& inputs() { return m_inputs; }

	const std::vector<Gate*>& get_expanded() const;
	std::string get_str() const;

	Line* get_output() const { return m_output; }
	Line*& output() { return m_output; }

	size_t get_id() const { return m_id; }
	const IdMaker& get_id_maker() const { return m_id_maker; }

private:
	IdMaker& m_id_maker;
	Type m_type = Type::Undefined;
	std::vector<Line*> m_inputs;
	Line* m_output = nullptr;

	size_t m_id = std::numeric_limits<size_t>::max();

	std::vector<Gate*> m_expanded_gate_ptrs;

	std::deque<Gate> m_expanded_gate;
	std::deque<Line> m_expanded_lines;
};

template<typename Func>
void walk_gates_breadth_first(const std::vector<const Gate*>& from, Func func, bool toward_outputs = true, bool expand_gates = false)
{
	if (from.empty()) {
		return;
	}

	IdObjectSet<const Gate*> walked_gates(from.front()->get_id_maker().gate_id_end());

	std::deque<const Gate*> queue;

	auto add_to_walk = [&walked_gates, &queue](const Gate* gate) {
		if (!gate || walked_gates.count(gate)) {
			return;
		}
		walked_gates.insert(gate);
		queue.push_back(gate);
	};

	for (const Gate* gate : from) {
		assert(gate);
		add_to_walk(gate);
	}
	while (!queue.empty()) {
		const Gate* gate = queue.front();
		queue.pop_front();

		if (!expand_gates) {
			func(gate);
		} else {
			for (const Gate* expanded_gate : gate->get_expanded()) {
				func(expanded_gate);
			}
		}

		if (toward_outputs) {
			for (const Gate* dest : gate->get_output()->destination_gates) {
				add_to_walk(dest);
			}
		} else {
			for (const Line* input : gate->get_inputs()) {
				assert(input);
				add_to_walk(input->source);
			}
		}
	}
}

class CircuitGraph : public IdMaker
{
public:
	Line* add_input(const std::string& name);
	Line* add_output(const std::string& name);

	Gate* add_gate(Gate::Type type, const std::vector<std::string>& input_names, const std::string& output_name);

	Line* get_line(const std::string& name);

	const Line* get_line(const std::string& name) const;

	const std::vector<Line*>& get_inputs() const;
	const std::vector<Line*>& get_outputs() const;

	const std::deque<Gate>& get_gates() const;
	const std::deque<Line>& get_lines() const;

	std::string get_graph_stats() const;

private:
	Line* ensure_line(const std::string& name);

	// We need to avoid relocations on element addition, hence deque
	std::deque<Line> m_lines;
	std::deque<Gate> m_gates;

	std::vector<Line*> m_inputs;
	std::vector<Line*> m_outputs;

	std::unordered_map<std::string, Line*> m_name_to_line;
};

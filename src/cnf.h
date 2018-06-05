#pragma once

#include <vector>
#include <set>
#include <cstddef>
#include <cstdint>
#include <string>
#include <array>

using literal_t = int;

struct clause_t : public std::vector<literal_t>
{
	using std::vector<literal_t>::vector;

	clause_t()
	{
		reserve(8);
	}
};

// uint8_t because vector of bools is a slow specialization that should be ashamed of itself
using assignment_t = std::vector<uint8_t>;

class Cnf;
struct ICnf
{
	virtual ~ICnf() = default;

	virtual ICnf& operator=(const Cnf& other) = 0;
	virtual void reserve(size_t size) = 0;
	virtual void clear() = 0;
	virtual void add_clause(clause_t clause) = 0;
	virtual void add_clause(literal_t l1, literal_t l2 = 0, literal_t l3 = 0, literal_t l4 = 0, literal_t l5 = 0) = 0;
	virtual void add_clauses(std::vector<clause_t>& from) = 0;
};

class Cnf : public ICnf
{
public:
	Cnf() = default;
	Cnf(const Cnf& other) = default;

	Cnf& operator=(const Cnf& other) override final
	{
		m_clauses = other.m_clauses;
		return *this;
	}

	Cnf& operator=(Cnf&& other)
	{
		m_clauses = std::move(other.m_clauses);
		return *this;
	}

	void reserve(size_t size) override final
	{
		m_clauses.reserve(size);
	}

	void clear() override final
	{
		m_clauses.clear();
	}

	void add_clause(clause_t clause) override final;
	void add_clause(literal_t l1, literal_t l2 = 0, literal_t l3 = 0, literal_t l4 = 0, literal_t l5 = 0) override final;
	void add_clauses(std::vector<clause_t>& from) override final;

	bool is_satisfied(const assignment_t& assignement) const;

	const std::vector<clause_t>& get_clauses() const { return m_clauses; }

	std::string get_dimacs_str() const;

private:
	std::vector<clause_t> m_clauses;
};

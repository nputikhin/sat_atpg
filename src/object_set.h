#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

template<typename T>
class IdObjectSet
{
public:
	IdObjectSet(size_t size)
	{
		m_vec.resize(size, 0);
	}

	void insert(const typename std::remove_pointer<T>::type* const obj)
	{
		assert(obj->get_id() < m_vec.size());
		m_vec[obj->get_id()] = 1;
	}

	void erase(const typename std::remove_pointer<T>::type* const obj)
	{
		assert(obj->get_id() < m_vec.size());
		m_vec[obj->get_id()] = 0;
	}

	size_t count(const typename std::remove_pointer<T>::type* const obj) const
	{
		assert(obj->get_id() < m_vec.size());
		return m_vec[obj->get_id()];
	}

private:
	std::vector<uint8_t> m_vec;
};

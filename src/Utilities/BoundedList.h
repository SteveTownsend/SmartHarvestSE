#pragma once

#include <vector>

template <typename MEMBER>
class BoundedList {
public:
	BoundedList(size_t capacity) : m_capacity(capacity) {
		m_data.reserve(capacity);
	}

	// adds new member and returns false iff the list is full
	bool Add(MEMBER member) {
		m_data.push_back(member);
		if (m_data.size() < m_capacity)
			return true;
		DBG_VMESSAGE("Bounded list reached capacity (%d)", m_capacity);
		return false;
	}

	std::vector<MEMBER>& Data() {
		return m_data;
	}

private:
	std::vector<MEMBER> m_data;
	size_t m_capacity;
};
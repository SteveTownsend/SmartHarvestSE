#pragma once

class ExcludedLocations
{
public:
	static ExcludedLocations& Instance();
	ExcludedLocations() {}

	void Reset();
	void Add(const RE::TESForm* location);
	void Drop(const RE::TESForm* location);
	bool Contains(const RE::TESForm* location) const;

private:
	static std::unique_ptr<ExcludedLocations> m_instance;

	std::unordered_set<const RE::TESForm*> m_excluded;
	mutable RecursiveLock m_excludedLock;
};

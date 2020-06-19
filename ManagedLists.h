#pragma once

// blacklist and whitelist, can contain places and items
class ManagedList
{
public:
	static ManagedList& BlackList();
	static ManagedList& WhiteList();
	ManagedList() {}

	void Reset(const bool reloadGame);
	void Add(const RE::TESForm* entry);
	void Drop(const RE::TESForm* entry);
	bool Contains(const RE::TESForm* entry) const;

private:
	static std::unique_ptr<ManagedList> m_blackList;
	static std::unique_ptr<ManagedList> m_whiteList;

	std::unordered_set<const RE::TESForm*> m_members;
	mutable RecursiveLock m_listLock;
};

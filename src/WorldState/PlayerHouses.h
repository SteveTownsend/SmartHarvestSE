#pragma once

class PlayerHouses
{
public:
	static PlayerHouses& Instance();
	PlayerHouses();

	void Clear();
	bool Add(const RE::BGSLocation* location);
	bool Remove(const RE::BGSLocation* location);
	bool Contains(const RE::BGSLocation* location) const;

	void SetKeyword(RE::BGSKeyword* keyword);
	bool IsValidHouse(const RE::BGSLocation* location) const;

private:
	static std::unique_ptr<PlayerHouses> m_instance;
	RE::BGSKeyword* m_keyword;

	std::unordered_set<const RE::BGSLocation*> m_houses;
	mutable RecursiveLock m_housesLock;
};

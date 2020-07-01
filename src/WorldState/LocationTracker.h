#pragma once

class LocationTracker
{
private:
	void RecordAdjacentCells(RE::TESObjectCELL* cell);
	bool IsAdjacent(RE::TESObjectCELL* cell) const;
	bool IsPlayerInWhitelistedPlace() const;
	bool IsPlayerInBlacklistedPlace() const;

	static std::unique_ptr<LocationTracker> m_instance;
	std::vector<RE::TESObjectCELL*> m_adjacentCells;
	RE::TESObjectCELL* m_playerCell;
	bool m_playerCellSelfOwned;
	RE::BGSLocation* m_playerLocation;
	RE::BGSLocation* m_priorLocation;
	mutable RecursiveLock m_locationLock;

public:
	static LocationTracker& Instance();
	LocationTracker();

	bool IsCellSelfOwned() const;
	RE::TESForm* GetCellOwner(RE::TESObjectCELL* cell) const;
	bool IsCellPlayerOwned(RE::TESObjectCELL* cell) const;
	void Reset(const bool reloaded);
	void Refresh();
	bool IsPlayerAtHome() const;
	bool IsPlayerInLootablePlace();
	decltype(m_adjacentCells) AdjacentCells() const;
	bool IsPlayerIndoors() const;
	bool IsPlayerInRestrictedLootSettlement() const;
	RE::TESForm* CurrentPlayerPlace() const;

	const RE::TESObjectCELL* PlayerCell() const;
};

#pragma once

class LocationTracker
{
private:
	void RecordAdjacentCells();
	bool IsAdjacent(RE::TESObjectCELL* cell) const;
	bool IsPlayerInBlacklistedPlace(const RE::TESObjectCELL* cell) const;

	static std::unique_ptr<LocationTracker> m_instance;
	std::vector<RE::TESObjectCELL*> m_adjacentCells;
	const RE::TESObjectCELL* m_priorCell;
	RE::TESObjectCELL* m_playerCell;
	bool m_tellPlayerIfCanLootAfterLoad;
	const RE::BGSLocation* m_playerLocation;
	mutable RecursiveLock m_locationLock;

public:
	static LocationTracker& Instance();
	LocationTracker();

	bool CellOwnedByPlayerOrPlayerFaction(const RE::TESObjectCELL* cell) const;
	RE::TESForm* GetCellOwner(const RE::TESObjectCELL* cell) const;
	void Reset();
	bool Refresh();
	bool IsPlayerAtHome() const;
	bool IsPlayerInLootablePlace(const RE::TESObjectCELL* cell, const bool lootableIfRestricted);
	decltype(m_adjacentCells) AdjacentCells() const;
	bool IsPlayerIndoors() const;
	bool IsPlayerInRestrictedLootSettlement(const RE::TESObjectCELL* cell) const;
	const RE::TESForm* CurrentPlayerPlace() const;
	bool IsPlayerInWhitelistedPlace(const RE::TESObjectCELL* cell) const;

	const RE::TESObjectCELL* PlayerCell() const;
};

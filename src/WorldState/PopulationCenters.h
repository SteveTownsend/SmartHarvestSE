#pragma once

// Population Center Looting Size, overloaded to check Looting Permissions
enum class PopulationCenterSize {
	None = 0,
	Settlements,
	Towns,		// implies Settlements
	Cities,		// implies Towns and Settlements
	MAX
};

class PopulationCenters
{
public:
	static PopulationCenters& Instance();
	PopulationCenters() {}

	void Categorize();
	bool CannotLoot(const RE::BGSLocation* location) const;

private:
	PopulationCenterSize PopulationCenterSizeFromIniSetting(const double iniSetting) const;

	static std::unique_ptr<PopulationCenters> m_instance;

	std::unordered_map<const RE::BGSLocation*, PopulationCenterSize> m_centers;
	mutable RecursiveLock m_centersLock;
};

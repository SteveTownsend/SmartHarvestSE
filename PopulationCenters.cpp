#include "PrecompiledHeaders.h"
#include "PopulationCenters.h"

std::unique_ptr<PopulationCenters> PopulationCenters::m_instance;

PopulationCenters& PopulationCenters::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<PopulationCenters>();
	}
	return *m_instance;
}

bool PopulationCenters::CannotLoot(const RE::BGSLocation* location) const
{
	PopulationCenterSize excludedCenterSize(PopulationCenterSizeFromIniSetting(
		INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::common, INIFile::SecondaryType::config, "PreventPopulationCenterLooting")));
	if (excludedCenterSize == PopulationCenterSize::None)
		return false;

	RecursiveLockGuard guard(m_centersLock);
	const auto locationRecord(m_centers.find(location));
	// if small locations are excluded we automatically exclude any larger, so use >= here, assuming this is
	// a population center
	return locationRecord != m_centers.cend() && locationRecord->second >= excludedCenterSize;
}

PopulationCenterSize PopulationCenters::PopulationCenterSizeFromIniSetting(const double iniSetting) const
{
	UInt32 intSetting(static_cast<UInt32>(iniSetting));
	if (intSetting >= static_cast<SInt32>(PopulationCenterSize::MAX))
	{
		return PopulationCenterSize::Cities;
	}
	return static_cast<PopulationCenterSize>(intSetting);
}

// Classify items by their keywords
void PopulationCenters::Categorize()
{
	RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
	if (!dhnd)
		return;

	std::unordered_map<std::string, PopulationCenterSize> sizeByKeyword =
	{
		// Skyrim core
		{"LocTypeSettlement", PopulationCenterSize::Settlements},
		{"LocTypeTown", PopulationCenterSize::Towns},
		{"LocTypeCity", PopulationCenterSize::Cities}
	};

	for (RE::BGSLocation* location : dhnd->GetFormArray<RE::BGSLocation>())
	{
		// Scan location keywords to check if it's a settlement
		UInt32 numKeywords(location->GetNumKeywords());
		PopulationCenterSize size(PopulationCenterSize::None);
		std::string largestMatch;
		for (UInt32 next = 0; next < numKeywords; ++next)
		{
			std::optional<RE::BGSKeyword*> keyword(location->GetKeywordAt(next));
			if (!keyword.has_value() || !keyword.value())
				continue;

			std::string keywordName(FormUtils::SafeGetFormEditorID(keyword.value()));
			const auto matched(sizeByKeyword.find(keywordName));
			if (matched == sizeByKeyword.cend())
				continue;
			if (matched->second > size)
			{
				size = matched->second;
				largestMatch = keywordName;
			}
		}
		// record population center size in case looting is selectively prevented
		if (size != PopulationCenterSize::None)
		{
			DBG_MESSAGE("%s/0x%08x is population center of type %s", location->GetName(), location->GetFormID(), largestMatch.c_str());
			m_centers.insert(std::make_pair(location, size));
		}
		else
		{
			DBG_MESSAGE("%s/0x%08x is not a population center", location->GetName(), location->GetFormID());
		}
	}

	// We also categorize descendants of population centers. Not all will follow the same rule as the parent. For example,
	// preventing looting in Whiterun should also prevent looting in the Bannered Mare, but not in Whiterun Sewers. Use
	// child location keywords to control this.
	std::unordered_set<std::string> lootableChildLocations =
	{
		// not all Skyrim core, necessarily
		"LocTypeClearable",
		"LocTypeDungeon",
		"LocTypeDraugrCrypt",
		"LocTypeNordicRuin",
		"zzzBMLocVampireDungeon"
	};
#if _DEBUG
	std::unordered_set<std::string> childKeywords;
#endif
	for (RE::BGSLocation* location : dhnd->GetFormArray<RE::BGSLocation>())
	{
		// check if this is a descendant of a population center
		RE::BGSLocation* antecedent(location->parentLoc);
		PopulationCenterSize parentSize(PopulationCenterSize::None);
		while (antecedent != nullptr)
		{
			const auto matched(m_centers.find(antecedent));
			if (matched != m_centers.cend())
			{
				parentSize = matched->second;
				DBG_MESSAGE("%s/0x%08x is a descendant of population center %s/0x%08x with size %d", location->GetName(), location->GetFormID(),
					antecedent->GetName(), antecedent->GetFormID(), parentSize);
				break;
			}
			antecedent = antecedent->parentLoc;
		}

		if (!antecedent)
			continue;

		// Scan location keywords to determine if lootable, or bucketed with its population center antecedent
		UInt32 numKeywords(location->GetNumKeywords());
		bool allowLooting(false);
		for (UInt32 next = 0; !allowLooting && next < numKeywords; ++next)
		{
			std::optional<RE::BGSKeyword*> keyword(location->GetKeywordAt(next));
			if (!keyword.has_value() || !keyword.value())
				continue;

			std::string keywordName(keyword.value()->GetFormEditorID());
#if _DEBUG
			childKeywords.insert(keywordName);
#endif
			if (lootableChildLocations.find(keywordName) != lootableChildLocations.cend())
			{
				allowLooting = true;
				DBG_MESSAGE("%s/0x%08x is lootable child location due to keyword %s", location->GetName(), location->GetFormID(), keywordName.c_str());
				break;
			}
		}
		if (allowLooting)
			continue;

		// Store the child location with the same criterion as parent, unless it's inherently lootable
		// e.g. dungeon within the city limits like Whiterun Sewers, parts of the Ratway
		DBG_MESSAGE("%s/0x%08x stored with same rule as its parent population center", location->GetName(), location->GetFormID());
		m_centers.insert(std::make_pair(location, parentSize));
	}
#if _DEBUG
	// this debug output from a given load order drives the list of 'really lootable' child location types above
	for (const std::string& keyword : childKeywords)
	{
		DBG_MESSAGE("Population center child keyword: %s", keyword.c_str());
	}
#endif
}

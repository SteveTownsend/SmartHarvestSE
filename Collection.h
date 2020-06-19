#pragma once

namespace shse {

	class Collection {
public:
	Collection(const std::string& name, const std::string& description, std::unique_ptr<ConditionTree> filter);
	bool IsMemberOf(const RE::TESForm* form, const ObjectType objectType) const;
	void RecordNewMember(const RE::FormID itemID, const RE::TESForm* form);
	nlohmann::json MakeJSON() const;
	void AsJSON(nlohmann::json& j) const;

private:
	std::string m_name;
	std::string m_description;
	std::unique_ptr<ConditionTree> m_rootFilter;
	std::unordered_map<RE::FormID, const RE::TESForm*> m_members;
};

void to_json(nlohmann::json& j, const Collection& collection);

}

std::ostream& operator<<(std::ostream& os, const shse::Collection& collection);

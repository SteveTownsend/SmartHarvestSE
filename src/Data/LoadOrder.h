#pragma once

namespace shse
{

class LoadOrder {
public:
	static LoadOrder& Instance();
	bool Analyze(void);
	RE::FormID GetFormIDMask(const std::string& modName) const;
	bool IncludesMod(const std::string& modName) const;
	bool ModOwnsForm(const std::string& modName, const RE::FormID formID) const;

private:
	static constexpr RE::FormID LightFormIDSentinel = 0xfe000000;
	static constexpr RE::FormID LightFormIDMask = 0xfefff000;
	static constexpr RE::FormID RegularFormIDMask = 0xff000000;
	// no lock as all public functions are const once loaded
	static std::unique_ptr<LoadOrder> m_instance;
	std::unordered_map<std::string, RE::FormID> m_formIDMaskByName;
};

}

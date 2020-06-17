#pragma once

class LoadOrder {
public:
	static LoadOrder& Instance();
	bool Analyze(void);
	RE::FormID GetFormIDMask(const std::string& modName) const;
	bool IncludesMod(const std::string& modName) const;

private:
	static std::unique_ptr<LoadOrder> m_instance;
	std::unordered_map<std::string, RE::FormID> m_formIDMaskByName;
};

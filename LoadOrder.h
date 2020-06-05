#pragma once

class LoadOrder {
public:
	static LoadOrder& Instance();
	void Analyze(void);

private:
	static std::unique_ptr<LoadOrder> m_instance;
	std::unordered_map<std::string, RE::FormID> m_formIDMaskByName;
};

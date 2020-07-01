#pragma once

class ProducerLootables
{
private:
	static std::unique_ptr<ProducerLootables> m_instance;
	mutable RecursiveLock m_producerIngredientLock;
	std::unordered_map<RE::TESForm*, RE::TESForm*> m_producerLootable;

public:
	static ProducerLootables& Instance();
	ProducerLootables() {}

	bool SetLootableForProducer(RE::TESForm* critter, RE::TESForm* ingredient);
	RE::TESForm* GetLootableForProducer(RE::TESForm* producer) const;
};

#include "PrecompiledHeaders.h"
#include "Data/dataCase.h"
#include "ProducerLootables.h"

namespace shse
{

std::unique_ptr<ProducerLootables> ProducerLootables::m_instance;

ProducerLootables& ProducerLootables::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<ProducerLootables>();
	}
	return *m_instance;
}

// ingredient nullptr indicates this Producer is pending resolution
bool ProducerLootables::SetLootableForProducer(RE::TESForm* producer, RE::TESForm* lootable)
{
	RecursiveLockGuard guard(m_producerIngredientLock);
	if (!lootable)
	{
		DBG_VMESSAGE("Producer %s/0x%08x needs resolving to lootable", producer->GetName(), producer->formID);
		// return value signals entry pending resolution found/not found
		return m_producerLootable.insert(std::make_pair(producer, nullptr)).second;
	}
	else
	{
		// lootable/nonIngredientLootable could be a FormList possibly empty, or a simple Form: resolved by script
		if (!lootable)
		{
			REL_WARNING("Producer %s/0x%08x has no lootable", producer->GetName(), producer->formID);
			DataCase::GetInstance()->BlockForm(producer);
		}
		else
		{
			DBG_VMESSAGE("Producer %s/0x%08x has lootable %s/0x%08x", producer->GetName(), producer->formID, lootable->GetName(), lootable->formID);
			m_producerLootable[producer] = lootable;
		}
		return true;
	}
}

RE::TESForm* ProducerLootables::GetLootableForProducer(RE::TESForm* producer) const
{
	RecursiveLockGuard guard(m_producerIngredientLock);
	const auto matched(m_producerLootable.find(producer));
	if (matched != m_producerLootable.cend())
		return matched->second;
	return nullptr;
}

}
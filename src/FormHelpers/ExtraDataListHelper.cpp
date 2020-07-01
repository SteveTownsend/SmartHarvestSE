#include "PrecompiledHeaders.h"

#include "FormHelpers/ExtraDataListHelper.h"

RE::EnchantmentItem * ExtraDataListHelper::GetEnchantment(void)
{
	if (!m_extraData)
		return false;

	auto exEnchant = m_extraData->GetByType<RE::ExtraEnchantment>();
	return (exEnchant && exEnchant->enchantment) ? exEnchant->enchantment : nullptr;
}

bool ExtraDataListHelper::IsQuestObject(const bool requireFullQuestFlags)
{
	if (!m_extraData)
		return false;

	auto exAliasArray = m_extraData->GetByType<RE::ExtraAliasInstanceArray>();
	if (!exAliasArray)
		return false;

	return std::find_if(exAliasArray->aliases.cbegin(), exAliasArray->aliases.cend(),
		[=](const RE::BGSRefAliasInstanceData* alias) -> bool {
			if (alias->alias->IsQuestObject() || (!requireFullQuestFlags && alias->quest)) {
				DBG_VMESSAGE("Quest Item confirmed in alias for quest 0x%08x, alias quest object %s",
					alias->quest ? alias->quest->formID : 0, alias->alias->IsQuestObject() ? "true" : "false");
				return true;
			}
			return false;
		}) != exAliasArray->aliases.cend();
}


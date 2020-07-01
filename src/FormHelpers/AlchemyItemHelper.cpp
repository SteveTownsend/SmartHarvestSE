#include "PrecompiledHeaders.h"

#include "AlchemyItemHelper.h"

UInt32 AlchemyItemHelper::GetGoldValue() const
{
	if (!m_alchemyItem)
		return 0;

	if ((m_alchemyItem->data.flags & RE::AlchemyItem::AlchemyFlag::kCostOverride) == RE::AlchemyItem::AlchemyFlag::kCostOverride)
		return m_alchemyItem->data.costOverride;

	double costPP(0.0);
	for (RE::Effect* effect : m_alchemyItem->effects)
	{
		if (!effect)
			continue;
		costPP += effect->cost;
	}

	UInt32 result = std::max<UInt32>(static_cast<UInt32>(costPP), 0);
	return result;
}
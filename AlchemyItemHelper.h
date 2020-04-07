#pragma once

class AlchemyItemHelper
{
public:
	AlchemyItemHelper(const RE::AlchemyItem* alchemyItem) : m_alchemyItem(alchemyItem) {}
	UInt32 GetGoldValue(void) const;
private:
	const RE::AlchemyItem* m_alchemyItem;
};

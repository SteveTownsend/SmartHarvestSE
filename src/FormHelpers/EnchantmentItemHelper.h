#pragma once

class EnchantmentItemHelper
{
public:
	EnchantmentItemHelper(RE::EnchantmentItem* item) : m_item(item) {}
	UInt32 GetGoldValue(void);
private:
	RE::EnchantmentItem* m_item;
};

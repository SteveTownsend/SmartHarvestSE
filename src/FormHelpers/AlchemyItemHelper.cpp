/*************************************************************************
SmartHarvest SE
Copyright (c) Steve Townsend 2020

>>> SOURCE LICENSE >>>
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation (www.fsf.org); either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the GNU General Public License is available at
http://www.fsf.org/licensing/licenses
>>> END OF LICENSE >>>
*************************************************************************/
#include "PrecompiledHeaders.h"

#include "AlchemyItemHelper.h"

uint32_t AlchemyItemHelper::GetGoldValue() const
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

	uint32_t result = std::max<uint32_t>(static_cast<uint32_t>(costPP), 0);
	return result;
}
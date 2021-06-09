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
#pragma once

namespace shse
{
namespace ExtraDataList
{
	RE::EnchantmentItem* GetEnchantment(const RE::ExtraDataList* extraData);
	bool IsItemQuestObject(const RE::TESBoundObject* item, const RE::ExtraDataList* extraData);
	bool IsREFRQuestObject(const RE::TESObjectREFR* refr, const RE::ExtraDataList* extraData);
}
}

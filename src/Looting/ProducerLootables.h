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

#include "Data/LeveledItemCategorizer.h"

namespace shse
{

class ProducerLootables
{
private:
	static std::unique_ptr<ProducerLootables> m_instance;
	mutable RecursiveLock m_producerIngredientLock;
	std::unordered_map<const RE::TESForm*, RE::TESBoundObject*> m_producerLootable;

	class SyntheticFloraCategorizer : public LeveledItemCategorizer
	{
	public:
		SyntheticFloraCategorizer(const RE::TESObjectACTI* producer, const RE::TESLevItem* rootItem);
		inline ObjectType ContentsType() const { return m_contentsType; }

	protected:
		virtual void ProcessContentLeaf(RE::TESBoundObject* itemForm, ObjectType itemType) override;

	private:
		const RE::TESObjectACTI* m_producer;
		const RE::TESForm* m_contents;
		ObjectType m_contentsType;
	};

public:
	static ProducerLootables& Instance();
	ProducerLootables() {}

	bool SetLootableForProducer(const RE::TESForm* producer, RE::TESBoundObject* ingredient);
	bool ResolveLootableForProducer(RE::TESForm* producer, RE::TESLevItem* leveledItem);
	RE::TESBoundObject* GetLootableForProducer(RE::TESForm* producer) const;
};

}
#pragma once

#include <vector>
#include "skse64/GameForms.h"

double GetDistance(const RE::TESObjectREFR* refr, const double radius);

class TESObjectCELLHelper
{
private:
	enum
	{
		kFlag_Interior = 1,
		kFlag_Public = 0x20
	};

public:
	TESObjectCELLHelper(RE::TESObjectCELL* cell) : m_cell(cell) {}
	RE::TESForm* GetOwner(void);
	bool IsPlayerOwned(void);

	template<typename Fn>
	UInt32 GetReferences(std::vector<RE::TESObjectREFR*> *out, const double radius, Fn& fn)
	{
		if (m_cell && !m_cell->IsAttached())
			return 0;

    	UInt32 index = 0;
		UInt32 count = 0;
		RE::TESObjectREFR* refr = nullptr;
#if 1
		TESObjectCELL* skseCell(reinterpret_cast<TESObjectCELL*>(m_cell));
		for (UInt32 index = 0; index < skseCell->refData.maxSize; index++)
		{
			refr = reinterpret_cast<RE::TESObjectREFR*>(skseCell->refData.refArray[index].ref);
			if (refr && skseCell->refData.refArray[index].unk08)
#else
		// REFR iteration in cell misses "deadbodies" REFR that was looted prior to a reload
		for (const RE::NiPointer<RE::TESObjectREFR>& refptr : m_cell->references)
		{
			/* SKSE logic for TESObjectCELL has 'ref' as TESObjectREFR instance, unk08 is a sentinel value:

		      TESObjectREFR* refr = nullptr;
		      for (UInt32 index = 0; index < refData.maxSize; index++) {
			    refr = refData.refArray[index].ref;
			    if (refr && refData.refArray[index].unk08)
				...
			  }
			  The array member is GameForms.h->TESObjectCELL::ReferenceData::Reference.
			  In CommonLibSSE the 'refArray' member is BSTHashMap.h->BSTSet<>::
			  BSTScatterTableEntry.
			*/
			RE::TESObjectREFR* refr(refptr.get());
			if (refr)
#endif
			{
				if (!fn(refr))
					continue;

				double dist = GetDistance(refr, radius);
				if (dist > radius)
					continue;

				out->emplace_back(refr);
				count++;
			}
		}
		return count;
	}

	inline bool IsPublic(void) const {
		return (m_cell->cellFlags & RE::TESObjectCELL::Flag::kPublicArea) == RE::TESObjectCELL::Flag::kNone;
	}

	RE::TESObjectCELL* m_cell;
};

class GridCellArrayHelper
{
public:
	GridCellArrayHelper(const double radius) : m_radius(radius) {}
	UInt32 GetReferences(std::vector<RE::TESObjectREFR*> *out);
private:

	double m_radius;
};

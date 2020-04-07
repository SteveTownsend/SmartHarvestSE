#pragma once

#include <vector>

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
			{
				const RE::BSTSet<RE::NiPointer<RE::TESObjectREFR>>::entry_type& rawEntry(
					reinterpret_cast<const RE::BSTSet<RE::NiPointer<RE::TESObjectREFR>>::entry_type&>(refptr));
				if (!rawEntry.next)
					continue;

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
#if 0
// refactor or remove for good
class GridArray
{
public:
	virtual ~GridArray();

	virtual void Unk_01(void);
	virtual void Unk_02(void);
	virtual void Unk_03(void);
	virtual void Unk_04(void);
	virtual void Unk_05(void);
	virtual void Unk_06(void) = 0;
	virtual void Unk_07(void) = 0;
	virtual void Unk_08(void) = 0;
	virtual void Unk_09(void) = 0;
};

class GridCellArray : public GridArray
{
public:
	virtual ~GridCellArray();

	virtual void Unk_03(void) override;
	virtual void Unk_04(void) override;
	virtual void Unk_05(void) override;
	virtual void Unk_06(void) override;
	virtual void Unk_07(void) override;
	virtual void Unk_08(void) override;
	virtual void Unk_09(void) override;
	virtual void Unk_0A(void);

	RE::TESObjectCELL * Get(UInt32 x, UInt32 y);
	bool IsAttached(RE::TESObjectCELL *cell) const;
	inline UInt32 GetSize(void) { return size; }

	UInt32	unk04;
	UInt32	unk08;
	UInt32	size;
	RE::TESObjectCELL**	cells;
};

class TESForCells
{
public:
	virtual ~TESForCells();

	UInt32  unk04;  //04
	UInt32  unk08;  //08
	UInt32  unk0C;  //0C
	UInt32  unk10;  //10
	UInt32  unk14;  //14
	UInt32  unk18;  //18
	UInt32  unk1C;  //1C
	UInt32  unk20;  //20
	UInt32  unk24;  //24
	UInt32  unk28;  //28
	UInt32  unk2C;  //2C
	UInt32  unk30;  //30
	UInt32  unk34;  //34
	UInt32  unk38;  //38
	UInt32  unk3C;  //3C
	UInt32  unk40;  //40
	UInt32  unk44;  //44
	UInt32  unk48;  //48
	UInt32  unk4C;  //4C
	UInt32  unk50;  //50
	UInt32  unk54;  //54
	UInt32  unk58;  //58
	UInt32  unk5C;  //5C
	UInt32  unk60;  //60
	UInt32  unk64;  //64
	UInt32  unk68;  //68
	UInt32  unk6C;  //6C
	UInt32  unk70;  //70 
	GridCellArray* cellArray; // 74
	// ... 
};
#else
struct GridCellArray { };
#endif
class GridCellArrayHelper
{
public:
	GridCellArrayHelper(GridCellArray* cells, const double radius) : m_cells(cells), m_radius(radius) {}
	UInt32 GetReferences(std::vector<RE::TESObjectREFR*> *out);
private:

	GridCellArray* m_cells;
	double m_radius;
};

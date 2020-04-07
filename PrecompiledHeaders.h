#pragma once

// undefined CommonLibSSE dependency
namespace RE {
	class QueuedFile {
	public:
		int IncRef() {
			return 1;
		}
		int DecRef() {
			return 1;
		}
	};
}

#include "version.h"

#include "CommonLibSSE/include/ForceInclude.h"

#include "CommonLibSSE/include/SKSE/API.h"
#include "CommonLibSSE/include/SKSE/Interfaces.h"
#include "CommonLibSSE/include/SKSE/Logger.h"

#include "CommonLibSSE/include/RE/NiSmartPointer.h"
#include "CommonLibSSE/include/RE/NiTimeController.h"

#include "CommonLibSSE/include/RE/RTTI.h"
#include "CommonLibSSE/include/RE/Offsets_RTTI.h"

#include "CommonLibSSE/include/RE/BGSBaseAlias.h"
#include "CommonLibSSE/include/RE/BGSKeyword.h"
#include "CommonLibSSE/include/RE/BGSLocation.h"
#include "CommonLibSSE/include/RE/BGSLocationRefType.h"
#include "CommonLibSSE/include/RE/BGSNote.h"
#include "CommonLibSSE/include/RE/BGSPickupPutdownSounds.h"
#include "CommonLibSSE/include/RE/BGSSoundDescriptorForm.h"

#include "CommonLibSSE/include/RE/BSCoreTypes.h"
#include "CommonLibSSE/include/RE/BSFixedString.h"
#include "CommonLibSSE/include/RE/BSResourceNiBinaryStream.h"
#include "CommonLibSSE/include/RE/BSScaleformTranslator.h"

#include "CommonLibSSE/include/RE/BSTArray.h"
#include "CommonLibSSE/include/RE/BSTHashMap.h"

#include "CommonLibSSE/include/RE/TES.h"
#include "CommonLibSSE/include/RE/TESAmmo.h"
#include "CommonLibSSE/include/RE/TESContainer.h"
#include "CommonLibSSE/include/RE/TESDataHandler.h"
#include "CommonLibSSE/include/RE/TESFaction.h"
#include "CommonLibSSE/include/RE/TESFile.h"
#include "CommonLibSSE/include/RE/TESForm.h"
#include "CommonLibSSE/include/RE/TESFullName.h"
#include "CommonLibSSE/include/RE/TESLevItem.h"
#include "CommonLibSSE/include/RE/TESNPC.h"
#include "CommonLibSSE/include/RE/TESObjectACTI.h"
#include "CommonLibSSE/include/RE/TESObjectARMO.h"
#include "CommonLibSSE/include/RE/TESObjectBOOK.h"
#include "CommonLibSSE/include/RE/TESObjectCELL.h"
#include "CommonLibSSE/include/RE/TESObjectCONT.h"
#include "CommonLibSSE/include/RE/TESObjectLIGH.h"
#include "CommonLibSSE/include/RE/TESObjectMISC.h"
#include "CommonLibSSE/include/RE/TESObjectREFR.h"
#include "CommonLibSSE/include/RE/TESObjectTREE.h"
#include "CommonLibSSE/include/RE/TESObjectWEAP.h"
#include "CommonLibSSE/include/RE/TESRace.h"
#include "CommonLibSSE/include/RE/TESSound.h"
#include "CommonLibSSE/include/RE/TESValueForm.h"
#include "CommonLibSSE/include/RE/TESWeightForm.h"

#include "CommonLibSSE/include/RE/Actor.h"
#include "CommonLibSSE/include/RE/AlchemyItem.h"
#include "CommonLibSSE/include/RE/Effect.h"
#include "CommonLibSSE/include/RE/EffectSetting.h"
#include "CommonLibSSE/include/RE/EnchantmentItem.h"
#include "CommonLibSSE/include/RE/ExtraActivateRef.h"
#include "CommonLibSSE/include/RE/ExtraActivateRefChildren.h"
#include "CommonLibSSE/include/RE/ExtraAshPileRef.h"
#include "CommonLibSSE/include/RE/ExtraCharge.h"
#include "CommonLibSSE/include/RE/ExtraCount.h"
#include "CommonLibSSE/include/RE/ExtraLinkedRef.h"
#include "CommonLibSSE/include/RE/ExtraLocationRefType.h"
#include "CommonLibSSE/include/RE/ExtraLock.h"
#include "CommonLibSSE/include/RE/ExtraOwnership.h"
#include "CommonLibSSE/include/RE/ExtraContainerChanges.h"
#include "CommonLibSSE/include/RE/FormTraits.h"
#include "CommonLibSSE/include/RE/IngredientItem.h"
#include "CommonLibSSE/include/RE/InventoryChanges.h"
#include "CommonLibSSE/include/RE/InventoryEntryData.h"
#include "CommonLibSSE/include/RE/Misc.h"
#include "CommonLibSSE/include/RE/NiPoint3.h"
#include "CommonLibSSE/include/RE/PlayerCharacter.h"
#include "CommonLibSSE/include/RE/PlayerControls.h"
#include "CommonLibSSE/include/RE/ScrollItem.h"
#include "CommonLibSSE/include/RE/Setting.h"
#include "CommonLibSSE/include/RE/SkyrimVM.h"
#include "CommonLibSSE/include/RE/UI.h"

#include "CommonLibSSE/include/RE/BSScript/FunctionArguments.h"
#include "CommonLibSSE/include/RE/BSScript/ObjectTypeInfo.h"
#include "CommonLibSSE/include/RE/BSScript/PackUnpack.h"
#include "CommonLibSSE/include/RE/BSScript/Variable.h"
#include "CommonLibSSE/include/RE/BSScript/Internal/VirtualMachine.h"

#include "CommonLibSSE/include/RE/SkyrimScript/HandlePolicy.h"

#include "ObjectType.h"
#include "iniSettings.h"
#include "IHasValueWeight.h"
#include "TESFormHelper.h"
#include "utils.h"

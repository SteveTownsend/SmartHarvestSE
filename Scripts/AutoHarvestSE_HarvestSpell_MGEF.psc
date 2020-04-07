Scriptname AutoHarvestSE_HarvestSpell_MGEF extends ActiveMagicEffect  

import AutoHarvestSE

int type_AutoHarvest = 1
int type_Spell = 2

float g_Interval = 0.5
bool g_updateFlag = false
bool g_debugTrace = false

Event OnEffectStart(Actor aTarget, Actor akCaster)
	if (akCaster != Game.GetPlayer())
		return
	endif
	g_debugTrace = false

	g_updateFlag = true
	g_Interval = GetConfig_Interval(type_Spell)
	RegisterForSingleUpdate(g_Interval)
endEvent

Event OnUpdate()
	if (!g_updateFlag)
		return
	endif
	GetCloseReferences(type_Spell)
	RegisterForSingleUpdate(g_Interval)
endEvent

Event OnEffectFinish(Actor akTarget, Actor akCaster)
	if (akCaster != Game.GetPlayer())
		return
	endif
	g_updateFlag = false
	UnblockEverything()
endEvent


Scriptname SHSE_LootSense extends ActiveMagicEffect  
{Impart Colour-coded Glow to nearby loot}
import SHSE_PluginProxy

Event OnEffectStart(Actor akTarget, Actor akCaster)
	GlowNearbyLoot()
EndEvent
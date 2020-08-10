Scriptname SHSE_LocatePlayer extends ActiveMagicEffect  
{Trigger display of Player Location and Adventure Target}
import SHSE_PluginProxy

Event OnEffectStart(Actor akTarget, Actor akCaster)
	;DebugTrace("Fired Detect-Self Power")
	ShowLocation()
EndEvent
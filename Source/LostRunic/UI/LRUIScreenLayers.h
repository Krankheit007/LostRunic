/**
 * @file LRUIScreenLayers.h
 * @brief Defines the stable viewport stacking contract for Lost Runic screens.
 */
#pragma once

#include "UI/LRUITypes.h"

namespace LRUIScreenLayers
{
	inline int32 ResolveZOrder(const ELRScreenType screen)
	{
		switch (screen)
		{
		case ELRScreenType::StateOverlay:
			return 0;
		case ELRScreenType::HUD:
			return 5;
		case ELRScreenType::Narrative:
			return 10;
		case ELRScreenType::Dialogue:
			return 11;
		case ELRScreenType::Inventory:
		case ELRScreenType::Journal:
		case ELRScreenType::Collectibles:
			return 12;
		case ELRScreenType::Pause:
			return 13;
		case ELRScreenType::SaveSlots:
			return 14;
		case ELRScreenType::Transition:
			return 30;
		case ELRScreenType::None:
		default:
			return 0;
		}
	}
}

#pragma once

#include <ArcdpsExtension/arcdps_structs_slim.h>

enum class EventType
{
	Damage,
	Healing,
	SemiDamaging, // non-damaging events that still reset combat time
	Other,
};

static inline EventType GetEventType(const cbtevent* pEvent, bool pIsLocal)
{
	if (pEvent->is_statechange != 0 || pEvent->is_activation != 0 || pEvent->is_buffremove != 0)
	{
		return EventType::Other;
	}

	if (pEvent->buff == 0)
	{
		switch (pEvent->result)
		{
		case CBTR_NORMAL:
		case CBTR_CRIT:
		case CBTR_GLANCE:
			break;
		case CBTR_ACTIVATION:
			return EventType::Other;
		default:
			return EventType::SemiDamaging; // Breakbar / misc.
		}

		if ((pIsLocal && pEvent->value <= 0) || (!pIsLocal && pEvent->value >= 0))
		{
			return EventType::Damage; // Direct damage
		}
		else
		{
			return EventType::Healing; // Direct healing
		}
	}
	else
	{
		if (pEvent->buff_dmg == 0)
		{
			return EventType::SemiDamaging; // Buff apply
		}
		else if ((pIsLocal && pEvent->buff_dmg <= 0) || (!pIsLocal && pEvent->buff_dmg >= 0))
		{
			return EventType::Damage; // Buff damage
		}
		else
		{
			return EventType::Healing; // Buff healing (e.g. Regeneration)
		}
	}
}
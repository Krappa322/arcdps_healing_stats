#pragma once

#include <stdint.h>

/*
 * The reason these functions exist is to distinguish between direct healing and indirect healing. Some skills and
 * traits that do "percentage of damage dealt" based healing, for example Blood Reckoning and Invigorating Precision,
 * have their heal attributed to the skill that did the damage and not to the trait itself. We deal with this by
 * aggregating damage done by skills which are not normally supposed to heal under a pseudo-skill "Healing by Damage
 * Dealt".
 *
 * This has the edge case of lifestealing skills as well as skills which do both damage and heal, which are registered
 * on a per skill basis as "hybrid skills" inside of Skills.cpp. Non-registered skills of this category will
 * (incorrectly) show up as "Healing by Damage Dealt" even though there is no such effect active. There is no way to
 * distinguish between healing from hybrid skills and indirect healing, and as such the healing statistics for hybrid
 * skills can be shown as too high (and "Healing by Damage Dealt" shown as too low) when there are both hybrid skills
 * and indirect healing skills present.
 *
 * RegisterDamagingSkill is called to signify that a skill does damage (called for every damage tick in area combat)
 * IsSkillIndirectHealing is called to check if a skill only does damage normally (and its presence is thus caused by
 *   indirect healing)
*/
void RegisterDamagingSkill(uint32_t pSkillId, const char* pSkillName);
bool IsSkillIndirectHealing(uint32_t pSkillId, const char* pSkillName);
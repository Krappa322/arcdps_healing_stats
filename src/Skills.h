#pragma once

#include <stdint.h>

#include <atomic>
#include <map>
#include <mutex>

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
class SkillTable
{
public:
	SkillTable();

	void RegisterDamagingSkill(uint32_t pSkillId, const char* pSkillName);
	void RegisterSkillName(uint32_t pSkillId, const char* pSkillName);
	bool IsSkillIndirectHealing(uint32_t pSkillId, const char* pSkillName);

	// Returns the name for a given skill. The vast majority of skills will use the default skill name provided by
	// ArcDPS; a select few skills we override the name for, either because it's not mapped, it's incorrect, or because
	// the name is shared with another skill id (for example, Signet of Ether has two components to it - active and
	// passive).
	const char* GetSkillName(uint32_t pSkillId);

	std::map<uint32_t, const char*> GetState();

private:
	std::mutex mLock;
	std::map<uint32_t, const char*> mSkillNames;

	std::atomic<uint64_t> myDamagingSkills[(UINT16_MAX + 1) / 64];
	uint64_t myHybridSkills[(UINT16_MAX + 1) / 64];
};
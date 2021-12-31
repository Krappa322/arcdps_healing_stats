#include "Skills.h"

#include "Log.h"

#include <assert.h>
#include <Windows.h>

SkillTable::SkillTable()
	: myDamagingSkills{}
	, myHybridSkills{}
{
#define ENTRY(pId) myHybridSkills[pId / 64] |= (1ULL << (pId % 64))
	ENTRY(2654);  // Crashing Waves
	ENTRY(5510);  // Water Trident
	ENTRY(5549);  // Water Blast (Elementalist)
	ENTRY(5570);  // Signet of Water
	ENTRY(5595);  // Water Arrow
	ENTRY(9080);  // Leap of Faith
	ENTRY(9090);  // Symbol of Punishment (Writ of Persistence)
	ENTRY(9095);  // Symbol of Judgement (Writ of Persistence)
	ENTRY(9097);  // Symbol of Blades (Writ of Persistence)
	ENTRY(9108);  // Holy Strike
	ENTRY(9111);  // Symbol of Faith (Writ of Persistence)
	ENTRY(9140);  // Faithful Strike
	ENTRY(9143);  // Symbol of Swiftness (Writ of Persistence)
	ENTRY(9146);  // Symbol of Wrath (Writ of Persistence)
	ENTRY(9161);  // Symbol of Protection (Writ of Persistence)
	ENTRY(9192);  // Symbol of Spears (Writ of Persistence)
	ENTRY(9208);  // Symbol of Light (Writ of Persistence)
	ENTRY(9950);  // Nourishment (Blueberry Pie AND Slice of Rainbow Cake)
	ENTRY(9952);  // Nourishment (Strawberry Pie AND Cupcake)
	ENTRY(9954);  // Nourishment (Cherry Pie)
	ENTRY(9955);  // Nourishment (Blackberry Pie)
	ENTRY(9956);  // Nourishment (Mixed Berry Pie)
	ENTRY(9957);  // Nourishment (Omnomberry Pie AND Slice of Candied Dragon Roll)
	ENTRY(10190); // Cry of Frustration (Restorative Illusions)
	ENTRY(10191); // Mind Wrack (Restorative Illusions)
	ENTRY(10560); // Life Leech
	ENTRY(10563); // Life Siphon
	ENTRY(10594); // Life Transfer (Transfusion)
	ENTRY(10619); // Deadly Feast
	ENTRY(10643); // Gathering Plague (Transfusion)
	ENTRY(12424); // Blood Frenzy
	ENTRY(13684); // Lesser Symbol of Protection (Writ of Persistence)
	ENTRY(15259); // Nourishment (Omnomberry Ghost)
	ENTRY(21656); // Arcane Brilliance
	ENTRY(24800); // Nourishment (Prickly Pear Pie AND Bowl of Cactus Fruit Salad)
	ENTRY(26557); // Vengeful Hammers
	ENTRY(26646); // Battle Scars
	ENTRY(29145); // Mender's Rebuke
	ENTRY(29789); // Symbol of Energy (Writ of Persistence)
	ENTRY(29856); // Well of Recall (All's Well That Ends Well)
	ENTRY(30359); // Gravity Well (All's Well That Ends Well)
	ENTRY(30285); // Vampiric Aura
	ENTRY(30488); // "Your Soul is Mine!"
	ENTRY(30504); // Soul Spiral (Transfusion)
	ENTRY(30525); // Well of Calamity (All's Well That Ends Well)
	ENTRY(30783); // Wings of Resolve (Soaring Devastation) 
	ENTRY(30814); // Well of Action (All's Well That Ends Well)
	ENTRY(30864); // Tidal Surge
	ENTRY(33792); // Slice of Allspice Cake
	ENTRY(34207); // Nourishment (Scoop of Mintberry Swirl Ice Cream)
	ENTRY(37475); // Nourishment (Winterberry Pie)
	ENTRY(40624); // Symbol of Vengeance (Writ of Persistence)
	ENTRY(41052); // Sieche
	ENTRY(43199); // Breaking Wave
	ENTRY(44405); // Riptide
	ENTRY(44428); // Garish Pillar (Transfusion)
	ENTRY(45026); // Soulcleave's Summit
	ENTRY(45983); // Claptosis
	ENTRY(51646); // Transmute Frost
	ENTRY(51692); // Facet of Nature - Assassin
	ENTRY(56928); // Rewinder (Restorative Illusions)
	ENTRY(56930); // Split Second (Restorative Illusions)
	ENTRY(57117); // Nourishment (Salsa Eggs Benedict)
	ENTRY(57239); // Nourishment (Strawberry Cilantro Cheesecake) - Apparently this one has a separate id from the damage event
	ENTRY(57244); // Nourishment (Cilantro Lime Sous-Vide Steak)
	ENTRY(57253); // Nourishment (Coq Au Vin with Salsa)
	ENTRY(57267); // Nourishment (Mango Cilantro Creme Brulee)
	ENTRY(57269); // Nourishment (Salsa-Topped Veggie Flatbread)
	ENTRY(57295); // Nourishment (Clear Truffle and Cilantro Ravioli)
	ENTRY(57341); // Nourishment (Poultry Aspic with Salsa Garnish)
	ENTRY(57356); // Nourishment (Spherified Cilantro Oyster Soup)
	ENTRY(57401); // Nourishment (Fruit Salad with Cilantro Garnish)
	ENTRY(57409); // Nourishment (Cilantro and Cured Meat Flatbread)
#undef ENTRY

	std::lock_guard lock(mLock);

	// Fixing names
	mSkillNames.emplace(1066, "Revive"); // Pressing "f" on a downed person
	mSkillNames.emplace(13594, "Selfless Daring"); // The game maps this name incorrectly to "Selflessness Daring"
	mSkillNames.emplace(14024, "Natural Healing"); // The game does not map this one at all
	mSkillNames.emplace(26558, "Energy Expulsion");
	mSkillNames.emplace(29863, "Live Vicariously"); // The game maps this name incorrectly to "Vigorous Recovery"
	mSkillNames.emplace(30313, "Escapist's Fortitude"); // The game maps this to the wrong skill

	// Clarifying names that exist on more than one skill
	mSkillNames.emplace(21750, "Signet of the Ether (Active)");
	mSkillNames.emplace(21775, "Aqua Surge (Self)");
	mSkillNames.emplace(21776, "Aqua Surge (Area)");
	mSkillNames.emplace(26937, "Enchanted Daggers (Initial)");
	mSkillNames.emplace(28313, "Enchanted Daggers (Siphon)");
	mSkillNames.emplace(45686, "Breakrazor's Bastion (Self)");
	mSkillNames.emplace(46232, "Breakrazor's Bastion (Area)");
	mSkillNames.emplace(49103, "Signet of the Ether (Passive)");

	// Instant cast skills that might otherwise not be mapped in peer stats
	// 13594 is on this list as well, but we already override it above
	mSkillNames.emplace(40787, "Chapter 1: Desert Bloom");
	mSkillNames.emplace(41714, "Mantra of Solace");
}


void SkillTable::RegisterDamagingSkill(uint32_t pSkillId, const char* pSkillName)
{
	UNREFERENCED_PARAMETER(pSkillName);
	if (pSkillId > UINT16_MAX)
	{
		LOG("Too high skill id %u %s!", pSkillId, pSkillName);
		assert(false);
		return;
	}

	uint64_t prevVal = myDamagingSkills[pSkillId / 64].fetch_or(1ULL << (pSkillId % 64), std::memory_order_relaxed);
	if ((prevVal & (1ULL << (pSkillId % 64))) == 0)
	{
		LOG("Registered %u %s as damaging", pSkillId, pSkillName);
	}
}

void SkillTable::RegisterSkillName(uint32_t pSkillId, const char* pSkillName)
{
	std::lock_guard lock(mLock);

	auto [iter, inserted] = mSkillNames.try_emplace(pSkillId, pSkillName);
	if (inserted == true)
	{
		LOG("Registered skillname %u %s", pSkillId, pSkillName);
	}
}

bool SkillTable::IsSkillIndirectHealing(uint32_t pSkillId, const char* pSkillName)
{
	UNREFERENCED_PARAMETER(pSkillName);
	if (pSkillId > UINT16_MAX)
	{
		LOG("Too high skill id %u %s!", pSkillId, pSkillName);
		assert(false);
		return false;
	}

	// Check if the skill both heals and does damage. If so then treat it as a healing skill (even though other
	// traits/skills could've caused it to heal more)
	uint64_t val1 = myHybridSkills[pSkillId / 64];
	if ((val1 & (1ULL << (pSkillId % 64))) != 0)
	{
		return false;
	}

	// Otherwise if the skill only does damage normally then the healing is indirectly caused by another skill or trait
	uint64_t val2 = myDamagingSkills[pSkillId / 64].load(std::memory_order_relaxed);
	if ((val2 & (1ULL << (pSkillId % 64))) != 0)
	{
		return true;
	}

	// If the skill doesn't do any damage, then it's a pure healing skill
	return false;
}

const char* SkillTable::GetSkillName(uint32_t pSkillId)
{
	std::lock_guard lock(mLock);

	auto iter = mSkillNames.find(pSkillId);
	if (iter != mSkillNames.end())
	{
		return iter->second;
	}

	return nullptr;
}

std::map<uint32_t, const char*> SkillTable::GetState()
{
	std::map<uint32_t, const char*> result;
	{
		std::lock_guard lock(mLock);
		result = mSkillNames;
	}

	return result;
}

#include "Skills.h"

#include "Log.h"

#include <assert.h>
#include <Windows.h>

#include <atomic>

static std::atomic<uint64_t> IS_DAMAGING[(UINT16_MAX + 1) / 8];

// Declare it as a class so we can initialize it at compile time with constructor
class HybridSkillsTable
{
public:
	uint64_t Table[(UINT16_MAX + 1) / 8];

	constexpr HybridSkillsTable()
		: Table{}
	{
#define ENTRY(pId) Table[pId / 64] |= (1ULL << (pId % 64))

		ENTRY(2654);  // Crashing Waves
		ENTRY(5570);  // Signet of Water
		ENTRY(9950);  // Nourishment (Blueberry Pie AND Slice of Rainbow Cake)
		ENTRY(9952);  // Nourishment (Strawberry Pie AND Cupcake)
		ENTRY(9954);  // Nourishment (Cherry Pie)
		ENTRY(9955);  // Nourishment (Blackberry Pie)
		ENTRY(9956);  // Nourishment (Mixed Berry Pie)
		ENTRY(9957);  // Nourishment (Omnomberry Pie AND Slice of Candied Dragon Roll)
		ENTRY(10190); // Cry of Frustration (Restorative Illusions)
		ENTRY(10191); // Mind Wrack (Restorative Illusions)
		ENTRY(10563); // Life Siphon
		ENTRY(12424); // Blood Frenzy
		ENTRY(15259); // Nourishment (Omnomberry Ghost)
		ENTRY(21656); // Arcane Brilliance
		ENTRY(24800); // Nourishment (Prickly Pear Pie AND Bowl of Cactus Fruit Salad)
		ENTRY(26557); // Vengeful Hammers
		ENTRY(26646); // Battle Scars
		ENTRY(29145); // Mender's Rebuke
		ENTRY(29856); // Well of Recall (All's Well That Ends Well)
		ENTRY(30359); // Gravity Well (All's Well That Ends Well)
		ENTRY(30285); // Vampiric Aura
		ENTRY(30488); // "Your Soul is Mine!"
		ENTRY(30525); // Well of Calamity (All's Well That Ends Well)
		ENTRY(30814); // Well of Action (All's Well That Ends Well)
		ENTRY(33792); // Slice of Allspice Cake
		ENTRY(34207); // Nourishment (Scoop of Mintberry Swirl Ice Cream)
		ENTRY(37475); // Nourishment (Winterberry Pie)
		ENTRY(41052); // Sieche
		ENTRY(43199); // Breaking Wave
		ENTRY(44405); // Riptide
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
		ENTRY(57295); // Nourishment (Clear Truffle and Cilantro Ravioli)
		ENTRY(57341); // Nourishment (Poultry Aspic with Salsa Garnish)
		ENTRY(57356); // Nourishment (Spherified Cilantro Oyster Soup)
		ENTRY(57401); // Nourishment (Fruit Salad with Cilantro Garnish)
		ENTRY(57409); // Nourishment (Cilantro and Cured Meat Flatbread)

#undef ENTRY
	}
};

static constexpr HybridSkillsTable IS_HYBRID;

void RegisterDamagingSkill(uint32_t pSkillId, const char* pSkillName)
{
	if (pSkillId > UINT16_MAX)
	{
		LOG("Too high skill id %u %s!", pSkillId, pSkillName);
		assert(false);
		return;
	}

	uint64_t prevVal = IS_DAMAGING[pSkillId / 64].fetch_or(1ULL << (pSkillId % 64), std::memory_order_relaxed);
	if ((prevVal & (1ULL << (pSkillId % 64))) == 0)
	{
		LOG("Registered %u %s as damaging", pSkillId, pSkillName);
	}
}

bool IsSkillIndirectHealing(uint32_t pSkillId, const char* pSkillName)
{
	if (pSkillId > UINT16_MAX)
	{
		LOG("Too high skill id %u %s!", pSkillId, pSkillName);
		assert(false);
		return false;
	}

	// Check if the skill both heals and does damage. If so then treat it as a healing skill (even though other
	// traits/skills could've caused it to heal more)
	uint64_t val1 = IS_HYBRID.Table[pSkillId / 64];
	if ((val1 & (1ULL << (pSkillId % 64))) != 0)
	{
		return false;
	}

	// Otherwise if the skill only does damage normally then the healing is indirectly caused by another skill or trait
	uint64_t val2 = IS_DAMAGING[pSkillId / 64].load(std::memory_order_relaxed);
	if ((val2 & (1ULL << (pSkillId % 64))) != 0)
	{
		return true;
	}

	// If the skill doesn't do any damage, then it's a pure healing skill
	return false;
}

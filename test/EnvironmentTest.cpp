#pragma warning(push, 0)
#include <gtest/gtest.h>
#pragma warning(pop)

#include "Exports.h"

TEST(EnvironmentTest, shutdown_race)
{
	GlobalObjects::ALLOC_CONSOLE = false;

	ModInitSignature mod_init = get_init_addr("unit_test", nullptr, nullptr, GetModuleHandle(NULL), malloc, free);
	arcdps_exports exports = *mod_init();
	ASSERT_NE(exports.sig, 0);

	ag ag1{};
	ag ag2{};
	ag1.elite = 0;
	ag1.prof = static_cast<Prof>(1);
	ag2.self = 1;
	ag2.id = 100;
	ag2.name = "testagent.1234";
	uintptr_t res = exports.combat(nullptr, &ag1, &ag2, nullptr, 0, 0);
	ASSERT_EQ(res, 0);
	res = exports.combat_local(nullptr, &ag1, &ag2, nullptr, 0, 0);
	ASSERT_EQ(res, 0);

	ModReleaseSignature mod_release = get_release_addr();
	mod_release();

	res = exports.combat(nullptr, &ag1, &ag2, nullptr, 0, 0);
	ASSERT_EQ(res, 1);
	res = exports.combat_local(nullptr, &ag1, &ag2, nullptr, 0, 0);
	ASSERT_EQ(res, 1);
	res = exports.imgui(1);
	ASSERT_EQ(res, 1);
	res = exports.options_end();
	ASSERT_EQ(res, 1);
	res = exports.wnd_nofilter(0, 1, 0, 0);
	ASSERT_EQ(res, 0);
}
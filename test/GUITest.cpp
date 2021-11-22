#pragma warning(push, 0)
#include <gtest/gtest.h>
#pragma warning(pop)

#include "GUI.h"

TEST(GUITest, LoopDetection)
{
	HealTableOptions options;
	for (size_t i = 0; i < options.Windows.size(); i++)
	{
		options.Windows[0].WindowId = 100;
		options.Windows[0].AnchorWindowId = 200;
		options.Windows[1].WindowId = 200;
		options.Windows[1].AnchorWindowId = 300;
		options.Windows[2].WindowId = 300;
		options.Windows[2].AnchorWindowId = 0;

		options.Windows[3].WindowId = 1000;
		options.Windows[3].AnchorWindowId = 2000;
		options.Windows[4].WindowId = 2000;
		options.Windows[4].AnchorWindowId = 3000; // points to non-existant window

		options.Windows[5].WindowId = 10;
		options.Windows[5].AnchorWindowId = 20;
		options.Windows[6].WindowId = 20;
		options.Windows[6].AnchorWindowId = 30;
		options.Windows[7].WindowId = 30;
		options.Windows[7].AnchorWindowId = 40;
		options.Windows[8].WindowId = 40;
		options.Windows[8].AnchorWindowId = 20;

		FindAndResolveCyclicDependencies(options, i);
		if (i == 0 || i == 1 || i == 3 || i == 4)
		{
			// If non-cyclical chain, AnchorWindowId should not be reset
			ASSERT_NE(options.Windows[i].AnchorWindowId, 0);
		}
		else
		{
			// Else if no chain or if loop was detected, AnchorWindowId should be reset
			ASSERT_EQ(options.Windows[i].AnchorWindowId, 0);
		}
	}
}

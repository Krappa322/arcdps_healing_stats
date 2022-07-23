#pragma once
#include "UpdateCheckerBase.h"

class UpdateChecker final : public UpdateCheckerBase
{
public:
	void Log(std::string&& pMessage) override;
};

void Display_UpdateWindow();
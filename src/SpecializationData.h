#pragma once

#include <Windows.h>

#include <string>

struct SpecializationData
{
	std::string Abbreviation = "";
	UINT IconResourceId = 0;
	std::string IconName = "";
	size_t IconTextureId = 0;
	void* IconTextureData = nullptr;
};

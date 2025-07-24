#pragma once

#include <Windows.h>

#include <string>

struct SpecializationData
{
	std::string Abbreviation;
	UINT IconResourceId;
	size_t IconTextureId;
	void* IconTextureData;
};

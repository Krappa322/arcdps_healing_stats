#pragma once

#include <ArcdpsExtension/UpdateCheckerBase.h>

class UpdateChecker final : public ArcdpsExtension::UpdateCheckerBase
{
public:
	void Log(std::string&& pMessage) override;
	bool HttpDownload(const std::string& pUrl, const std::filesystem::path& pOutputFile) override;
	std::optional<std::string> HttpGet(const std::string& pUrl) override;
};

void Display_UpdateWindow();
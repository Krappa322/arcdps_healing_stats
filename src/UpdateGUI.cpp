#include "UpdateGUI.h"

#include "Exports.h"
#include "ImGuiEx.h"
#include "Log.h"

void UpdateChecker::Log(std::string&& pMessage)
{
	LogI("{}", pMessage);
}

void Display_UpdateWindow()
{
	auto& state = GlobalObjects::UPDATE_STATE;
	if (state == nullptr)
	{
		return;
	}

	std::lock_guard lock(state->Lock);
	if (state->UpdateStatus != UpdateChecker::Status::Unknown && state->UpdateStatus != UpdateChecker::Status::Dismissed)
	{
		bool shown = true;
		if (ImGui::Begin(
			"Healing Stats Update###HEALING_STATS_UPDATE",
			&shown,
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize) == true)
		{
			const UpdateChecker::Version& currentVersion = *state->CurrentVersion;
			const UpdateChecker::Version& newVersion = state->NewVersion;

			ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "A new update for the healing stats addon is available");
			ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "Current version: %u.%urc%u", currentVersion[0], currentVersion[1], currentVersion[2]);
			ImGui::TextColored(ImVec4(0.f, 1.f, 0.f, 1.f), "New version: %u.%urc%u", newVersion[0], newVersion[1], newVersion[2]);

			if (ImGui::Button("Open download page") == true)
			{
				ShellExecuteA(nullptr, nullptr, "https://github.com/Krappa322/arcdps_healing_stats/releases", nullptr, nullptr, SW_SHOW);
			}

			switch (state->UpdateStatus)
			{
			case UpdateChecker::Status::UpdateAvailable:
				if (ImGui::Button("Update automatically") == true)
				{
					GlobalObjects::UPDATE_CHECKER->PerformInstallOrUpdate(*state);
				}
				break;
			case UpdateChecker::Status::UpdateInProgress:
				ImGui::TextUnformatted("Update in progress");
				break;
			case UpdateChecker::Status::UpdateSuccessful:
				ImGui::TextColored(ImVec4(0.f, 1.f, 0.f, 1.f), "Update finished, restart Guild Wars 2 for the update to take effect");
				break;
			case UpdateChecker::Status::UpdateError:
				ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "An error occured while updating");
				break;
			default:
				break;
			}

			ImGui::End();
		}

		if (shown != true)
		{
			state->UpdateStatus = UpdateCheckerBase::Status::Dismissed;
		}
	}
}
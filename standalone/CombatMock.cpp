#ifdef STANDALONE
#include "CombatMock.h"

#include "../imgui/imgui.h"

void CombatMock::AddAgent(const char* pAgentName, const char* pAccountName, uint32_t pProfession, uint32_t pElite, uint8_t pSubgroup, uint16_t pMasterInstanceId)
{
	Agent& newAgent = myAgents.emplace_back();
	newAgent.AgentName = pAgentName;
	newAgent.AccountName = pAccountName;
	newAgent.Profession = pProfession;
	newAgent.Elite = pElite;
	newAgent.Subgroup = pSubgroup;
	newAgent.MasterInstanceId = pMasterInstanceId;

	newAgent.UniqueId = myNextUniqueId++;
	newAgent.InstanceId = 0;
}

void CombatMock::EnterCombat()
{
}

void CombatMock::DisplayWindow()
{
	ImGui::SetNextWindowSize(ImVec2(900, 70), ImGuiCond_Always);
	if (ImGui::Begin("Add agent", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar) == true)
	{
		ImGui::BeginTable("##AddAgent", 6, ImGuiTableFlags_SizingStretchSame, ImVec2(840, 70));

		for (const auto& label : { "Agent Name", "Account Name", "Profession", "Elite", "Subgroup", "Master Instance Id" })
		{
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(label);
		}

		ImGui::TableNextColumn();
		ImGui::PushItemWidth(840 / 6);
		ImGui::InputText("##AgentName", myInputAgentName, sizeof(myInputAgentName));
		ImGui::PopItemWidth();
		ImGui::TableNextColumn();
		ImGui::PushItemWidth(840 / 6);
		ImGui::InputText("##AccountName", myInputAccountName, sizeof(myInputAccountName));
		ImGui::PopItemWidth();
		ImGui::TableNextColumn();
		ImGui::PushItemWidth(840 / 6);
		ImGui::InputInt("##Profession", &myInputProfession, 0);
		ImGui::PopItemWidth();
		ImGui::TableNextColumn();
		ImGui::PushItemWidth(840 / 6);
		ImGui::InputInt("##Elite", &myInputElite, 0, 0, ImGuiInputTextFlags_CharsHexadecimal);
		ImGui::PopItemWidth();
		ImGui::TableNextColumn();
		ImGui::PushItemWidth(840 / 6);
		ImGui::InputInt("##Subgroup", &myInputSubgroup, 0);
		ImGui::PopItemWidth();
		ImGui::TableNextColumn();
		ImGui::PushItemWidth(840 / 6);
		ImGui::InputInt("##Master Instance Id", &myInputMasterInstanceId, 0);
		ImGui::PopItemWidth();

		ImGui::EndTable();

		ImGui::SameLine();
		if (ImGui::Button("Add", ImVec2(-1, -1)) == true)
		{
			AddAgent(myInputAgentName, myInputAccountName, static_cast<uint32_t>(myInputProfession), static_cast<uint32_t>(myInputElite), static_cast<uint8_t>(myInputSubgroup), static_cast<uint16_t>(myInputMasterInstanceId));
		}
	}
	ImGui::End();

	ImGui::SetNextWindowSize(ImVec2(900, -1), ImGuiCond_Always);
	if (ImGui::Begin("Agents", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar) == true)
	{
		ImGui::BeginTable("##AgentTable", 10, ImGuiTableFlags_SizingStretchProp);

		for (const auto& label : { "Agent Name", "Account Name", "Profession", "Elite", "Subgroup", "Master Instance Id", "Unique Id", "Instance Id", "Is Self" })
		{
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(label);
		}
		ImGui::TableNextColumn(); // empty label in remove column

		int i = 0;
		auto iter = myAgents.begin();
		while (iter != myAgents.end())
		{
			ImGui::PushID(i); // Needed for button to work

			ImGui::TableNextColumn();
			ImGui::TextUnformatted(iter->AgentName.c_str());
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(iter->AccountName.c_str());
			ImGui::TableNextColumn();
			ImGui::Text("%u", iter->Profession);
			ImGui::TableNextColumn();
			ImGui::Text("%x", iter->Elite);
			ImGui::TableNextColumn();
			ImGui::Text("%hhu", iter->Subgroup);
			ImGui::TableNextColumn();
			ImGui::Text("%hu", iter->MasterInstanceId);
			ImGui::TableNextColumn();
			ImGui::Text("%llu", iter->UniqueId);
			ImGui::TableNextColumn();
			ImGui::Text("%hu", iter->InstanceId);
			ImGui::TableNextColumn();
			ImGui::Text("%s", iter->UniqueId == mySelfId ? "true" : "false");

			ImGui::TableNextColumn();
			if (ImGui::SmallButton("remove") == true)
			{
				iter = myAgents.erase(iter);
			}
			else
			{
				iter++;
			}
			i++;

			ImGui::PopID();
		}

		ImGui::EndTable();
	}
	ImGui::End();

	ImGui::SetNextWindowSize(ImVec2(900, 70), ImGuiCond_Always);
	if (ImGui::Begin("Add event", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar) == true)
	{
		ImGui::BeginTable("##AddAgent", 6, ImGuiTableFlags_SizingStretchSame, ImVec2(840, 70));

		for (const auto& label : { "Agent Name", "Account Name", "Profession", "Elite", "Subgroup", "Master Instance Id" })
		{
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(label);
		}

		ImGui::TableNextColumn();
		ImGui::PushItemWidth(840 / 6);
		ImGui::InputText("##Skillname", myInputAgentName, sizeof(myInputAgentName));
		ImGui::PopItemWidth();
		ImGui::TableNextColumn();
		ImGui::PushItemWidth(840 / 6);
		ImGui::InputText("##Skillid", myInputAccountName, sizeof(myInputAccountName));
		ImGui::PopItemWidth();
		ImGui::TableNextColumn();
		ImGui::PushItemWidth(840 / 6);
		ImGui::InputInt("##Profession", &myInputProfession, 0);
		ImGui::PopItemWidth();
		ImGui::TableNextColumn();
		ImGui::PushItemWidth(840 / 6);
		ImGui::InputInt("##Elite", &myInputElite, 0, 0, ImGuiInputTextFlags_CharsHexadecimal);
		ImGui::PopItemWidth();
		ImGui::TableNextColumn();
		ImGui::PushItemWidth(840 / 6);
		ImGui::InputInt("##Subgroup", &myInputSubgroup, 0);
		ImGui::PopItemWidth();
		ImGui::TableNextColumn();
		ImGui::PushItemWidth(840 / 6);
		ImGui::InputInt("##Master Instance Id", &myInputMasterInstanceId, 0);
		ImGui::PopItemWidth();

		ImGui::EndTable();

		ImGui::SameLine();
		if (ImGui::Button("Add", ImVec2(-1, -1)) == true)
		{
			AddAgent(myInputAgentName, myInputAccountName, static_cast<uint32_t>(myInputProfession), static_cast<uint32_t>(myInputElite), static_cast<uint8_t>(myInputSubgroup), static_cast<uint16_t>(myInputMasterInstanceId));
		}
	}
	ImGui::End();
}
#endif
#include "application.h"

using namespace application;

Application::Application(Game&& game, loot_type_info::LootTypeInfo&& type_info, const std::string& state_file, int save_state_period)
	: game_(std::move(game)), loot_type_info_(std::move(type_info)), state_file_(state_file), save_state_period_(save_state_period_) {
}

std::pair<PlayerToken, size_t> Application::JoinGame(const Map* map, std::string& name) {
	return game_.AddPlayer(map, name);
}

const Map* Application::GetMap(const std::string& id) const noexcept {
	return game_.GetMap(id);
}

const std::vector<Map>& Application::GetMaps() const noexcept {
	return game_.GetMaps();
}

Player* Application::GetPlayer(const PlayerToken& token) {
	return game_.GetPlayer(token);
}

void Application::ProcessTime(int time) {
	game_.ProcessTimeMovement(time);

	if (save_state_period_ != -1) {
		accumulated_time_ += time;
		if (accumulated_time_ >= save_state_period_) {
			SaveGame();
			accumulated_time_ = 0;
		}
	}
}

void Application::SaveGame() {
	if (state_file_.empty()) {
		return;
	}

	std::ofstream ofs(state_file_);
	serialization::GameSerialization game_ser = serialization::GameSerialization::FromGame(game_);
	boost::archive::text_oarchive oa(ofs);
	oa << game_ser;

	ofs.close();
}

const loot_type_info::LootTypeInfo& Application::GetLootTypeInfo() const {
	return loot_type_info_;
}
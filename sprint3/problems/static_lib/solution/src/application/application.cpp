#include "application.h"

using namespace application;

Application::Application(Game&& game, loot_type_info::LootTypeInfo&& type_info) 
	: game_(std::move(game)), loot_type_info_(std::move(type_info)) {

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
}

const loot_type_info::LootTypeInfo& Application::GetLootTypeInfo() const {
	return loot_type_info_;
}
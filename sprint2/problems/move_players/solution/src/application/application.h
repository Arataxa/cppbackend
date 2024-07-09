#pragma once

#include "game.h"
#include "map.h"


namespace application {
	using namespace game;
	using namespace map;

	class Application {
	public:
		Application(Game&& game) : game_(std::move(game)) {

		}

		std::pair<PlayerToken, size_t> JoinGame(const Map* map, std::string& name) {
			return game_.AddPlayer(map, name);
		}

		const Map* GetMap(const std::string& id) const noexcept {
			return game_.GetMap(id);
		}

		const std::vector<Map>& GetMaps() const noexcept {
			return game_.GetMaps();
		}

		Player* GetPlayer(const PlayerToken& token) {
			return game_.GetPlayer(token);
		}

	private:
		Game game_;
	};
} // namespace application
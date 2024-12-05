#pragma once

#include "game.h"
#include "map.h"

#include <iostream>

namespace application {
	using namespace game;
	using namespace map;

	class Application {
	public:
		Application(Game&& game);

		std::pair<PlayerToken, size_t> JoinGame(const Map* map, std::string& name);

		const Map* GetMap(const std::string& id) const noexcept;

		const std::vector<Map>& GetMaps() const noexcept;

		Player* GetPlayer(const PlayerToken& token);

		void ProcessTime(double time);

	private:
		Game game_;
	};
} // namespace application
#pragma once

namespace application {
	namespace game {
		namespace utils {
            struct Coordinates {
                double x = 0.0;
                double y = 0.0;
            };

            enum Direction {
                NORTH, SOUTH, WEST, EAST
            };

            struct Speed {
                double x = 0.0;
                double y = 0.0;
            };
		}
	}
}
#include "loot.h"

namespace application {
	namespace game {
		namespace loot {
                CollectionResult TryCollectPoint(const Coordinates& a, const Coordinates& b, const Coordinates& c) {
                    assert(b.x != a.x || b.y != a.y);
                    const double u_x = c.x - a.x;
                    const double u_y = c.y - a.y;
                    const double v_x = b.x - a.x;
                    const double v_y = b.y - a.y;
                    const double u_dot_v = u_x * v_x + u_y * v_y;
                    const double u_len2 = u_x * u_x + u_y * u_y;
                    const double v_len2 = v_x * v_x + v_y * v_y;
                    const double proj_ratio = u_dot_v / v_len2;
                    const double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;

                    return CollectionResult(sq_distance, proj_ratio);
                }
		} // namespace loot
	} // namespace game
} // namespace application
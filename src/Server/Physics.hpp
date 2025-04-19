#pragma once

#include "../Shared/Protocol.hpp"

namespace Jetpack::Server {
class Physics {
public:
  static void applyPhysics(Shared::Protocol::Player &player);
  static void checkBounds(Shared::Protocol::Player &player,
                          const Shared::Protocol::GameMap &map);

private:
  static constexpr float GRAVITY = 0.008f;
  static constexpr float JETPACK_FORCE = 0.013f;
  static constexpr float MAX_VELOCITY = 0.05f;
  static constexpr float HORIZONTAL_SPEED = 0.05f;
};
} // namespace Jetpack::Server

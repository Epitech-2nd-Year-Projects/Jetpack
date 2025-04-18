#include "Physics.hpp"
#include <algorithm>

void Jetpack::Server::Physics::applyPhysics(Shared::Protocol::Player &player) {
  player.setVelocityY(player.getVelocityY() + GRAVITY);

  if (player.isJetpacking()) {
    player.setVelocityY(player.getVelocityY() - JETPACK_FORCE);
  }

  player.setVelocityY(
      std::clamp(player.getVelocityY(), -MAX_VELOCITY, MAX_VELOCITY));

  player.setPosition(player.getPosition().x + HORIZONTAL_SPEED,
                     player.getPosition().y + player.getVelocityY());
}

void Jetpack::Server::Physics::checkBounds(
    Shared::Protocol::Player &player, const Shared::Protocol::GameMap &map) {
  if (player.getPosition().y < 0) {
    player.setPosition(player.getPosition().x, 0);
    player.setVelocityY(0);
  } else if (player.getPosition().y >= map.height) {
    player.setPosition(player.getPosition().x, map.height - 0.1f);
    player.setVelocityY(0);
  }
}

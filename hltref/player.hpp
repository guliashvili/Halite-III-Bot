#pragma once

#include "dropoff.hpp"
#include "ship.hpp"
#include "shipyard.hpp"
#include "types.hpp"

#include <memory>
#include <vector>

using namespace std;

namespace hlt {
struct Player {
  PlayerId id;
  std::shared_ptr<Shipyard> shipyard;
  Halite halite;
  std::vector<std::shared_ptr<Ship>> ships;
  std::vector<std::shared_ptr<Dropoff>> dropoffs;
  std::vector<std::shared_ptr<Entity>> all_dropoffs;

  Player(PlayerId player_id, int shipyard_x, int shipyard_y)
      : id(player_id),
        shipyard(std::make_shared<Shipyard>(player_id, shipyard_x, shipyard_y)),
        halite(0) {
    all_dropoffs.push_back(shipyard);
  }

  void _update(int num_ships, int num_dropoffs, Halite halite);
  static std::shared_ptr<Player> _generate();
};
} // namespace hlt

#include "player.hpp"
#include "input.hpp"

void hlt::Player::_update(int num_ships, int num_dropoffs, Halite halite) {
  this->halite = halite;

  all_dropoffs.resize(1 + num_dropoffs);
  all_dropoffs[0] = this->shipyard;

  ships.resize(num_ships);
  for (int i = 0; i < num_ships; ++i) {
    ships[i] = hlt::Ship::_generate(id);
  }

  dropoffs.resize(num_dropoffs);
  for (int i = 0; i < num_dropoffs; ++i) {
    dropoffs[i] = hlt::Dropoff::_generate(id);
    all_dropoffs[i + 1] = dropoffs[i];
  }
}

std::shared_ptr<hlt::Player> hlt::Player::_generate() {
  PlayerId player_id;
  int shipyard_x;
  int shipyard_y;
  hlt::get_sstream() >> player_id >> shipyard_x >> shipyard_y;

  return std::make_shared<hlt::Player>(player_id, shipyard_x, shipyard_y);
}

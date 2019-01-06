#pragma once

#include "types.hpp"
#include "position.hpp"
#include "ship.hpp"
#include "dropoff.hpp"

namespace hlt {
    struct MapCell {
        Position position;
        Halite halite;
        std::shared_ptr<Ship> ship;
        std::shared_ptr<Entity> structure; // only has dropoffs and shipyards; if id is -1, then it's a shipyard, otherwise it's a dropoff
        bool collision;

        MapCell(int x, int y, Halite halite) :
            position(x, y),
            halite(halite), collision(false)
        {}

        bool is_empty() const {
            return !ship && !structure;
        }

        bool is_occupied() const {
            return static_cast<bool>(ship);
        }

        bool has_structure() const {
            return static_cast<bool>(structure);
        }

        void mark_unsafe(const std::shared_ptr<Ship>& ship) {
            collision |= this->ship != nullptr && this->ship != ship;
            this->ship = ship;
        }

        int extract_halite() const{
          return (halite + constants::EXTRACT_RATIO - 1)/constants::EXTRACT_RATIO;
        }

        int move_cost() const{
          return halite/constants::MOVE_COST_RATIO;
        }
        void mark_safe(){
          ship = nullptr;
          collision = false;
        }
    };
}

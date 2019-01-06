#pragma once

#include "types.hpp"
#include "map_cell.hpp"
#include "constants.hpp"
#include "genes.hpp"

#include <vector>
using namespace std;

namespace hlt {
    struct GameMap {
        int width;
        int height;
        std::vector<std::vector<MapCell>> cells;
        PlayerId me = -1;
        shared_ptr<Genes> genes;

        void init(PlayerId me_, shared_ptr<Genes> genes_){
          me = me_;
          genes = genes_;
        }

        MapCell* at(const Position& position) {
            return &cells[position.y][position.x];
        }

        inline MapCell* at(const Entity& entity) {
            return at(entity.position);
        }

        inline MapCell* at(const Entity* entity) {
            return at(entity->position);
        }

        inline MapCell* at(const std::shared_ptr<Entity>& entity) {
            return at(entity->position);
        }

        int calculate_distance(const Position& source, const Position& target) {
            const int dx = std::abs(source.x - target.x);
            const int dy = std::abs(source.y - target.y);

            const int toroidal_dx = std::min(dx, width - dx);
            const int toroidal_dy = std::min(dy, height - dy);

            return toroidal_dx + toroidal_dy;
        }

        std::vector<Direction> get_unsafe_moves(const Position& source, const Position& destination) {
            const int dx = std::abs(source.x - destination.x);
            const int dy = std::abs(source.y - destination.y);
            const int wrapped_dx = width - dx;
            const int wrapped_dy = height - dy;

            std::vector<Direction> possible_moves;

            if (source.x < destination.x) {
                possible_moves.push_back(dx > wrapped_dx ? Direction::WEST : Direction::EAST);
            } else if (source.x > destination.x) {
                possible_moves.push_back(dx < wrapped_dx ? Direction::WEST : Direction::EAST);
            }

            if (source.y < destination.y) {
                possible_moves.push_back(dy > wrapped_dy ? Direction::NORTH : Direction::SOUTH);
            } else if (source.y > destination.y) {
                possible_moves.push_back(dy < wrapped_dy ? Direction::NORTH : Direction::SOUTH);
            }

            return possible_moves;
        }

        bool has_my_structure(const Position& position){
          const auto& cell = at(position);
          return cell->has_structure() && cell->structure->owner == me;
        }


        Position _min_halite_next_pos;
        int _get_min_halite_enemy(const Position& position){
          int mn = 9999;
          for(auto direction : ALL_CARDINALS){
            position.directional_offset(_min_halite_next_pos, direction);
            if(at(_min_halite_next_pos)->is_occupied() && at(_min_halite_next_pos)->ship->owner != me){
                mn = min(mn, at(_min_halite_next_pos)->ship->halite);
            }
          }

          return mn;
        }

        bool is_safe(Position& pos, int halite=0, bool recall=false){
          if(!at(pos)->is_occupied() || (at(pos)->ship->owner != me && has_my_structure(pos)) || (recall &&  has_my_structure(pos))){
            if(halite - _get_min_halite_enemy(pos) < genes->collision_caution_margin){
              return true;
            }
          }
          return false;
        }

        Position _safe_moves_position;
        std::vector<Direction> get_safe_moves(shared_ptr<Ship> ship, const Position& destination, bool recall=false) {
          auto directions = get_unsafe_moves(ship->position, destination);
          std::vector<Direction> safe_directions;

          for(auto direction : directions){
            ship->position.directional_offset(_safe_moves_position, direction);
            if(is_safe(_safe_moves_position, ship->halite, recall)){
              safe_directions.push_back(direction);
            }
          }
          return safe_directions;
        }

        std::vector<Direction> get_safe_moves_around(shared_ptr<Ship> ship, bool recall=false) {
          std::vector<Direction> safe_directions;

          for(auto direction : ALL_CARDINALS){
            ship->position.directional_offset(_safe_moves_position, direction);
            if(is_safe(_safe_moves_position, ship->halite, recall)){
              safe_directions.push_back(direction);
            }
          }
          return safe_directions;
        }

        void navigate(std::shared_ptr<Ship> ship, const Position& destination) {
            // get_unsafe_moves normalizes for us
            at(destination)->mark_unsafe(ship);

            #ifdef DEBUG
            //TODO
            #endif
        }
        void navigate(std::shared_ptr<Ship> ship, const Direction& direction) {
            // get_unsafe_moves normalizes for us
            navigate(ship, ship->position.directional_offset(direction));
        }

        bool can_move(std::shared_ptr<Ship> ship){
            return at(ship->position)->halite / constants::MOVE_COST_RATIO <= ship->halite;
        }

        void _update();
        static std::unique_ptr<GameMap> _generate();
    };
}
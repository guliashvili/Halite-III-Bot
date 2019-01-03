#pragma once

#include "types.hpp"
#include "direction.hpp"
#include "constants.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

using namespace std;

namespace hlt {
    struct Position {
        int x;
        int y;

        Position() : x(0), y(0) {}
        Position(int x, int y) : x(x), y(y) {}

        bool operator==(const Position& other) const { return x == other.x && y == other.y; }
        bool operator!=(const Position& other) const { return x != other.x || y != other.y; }

        // strict weak ordering, useful for non-hash-based maps
        bool operator<(const Position &other) const {
            if (y != other.y) {
                return y < other.y;
            }
            return x < other.x;
        }

        std::string to_string() const {
            return std::to_string(x) + ':' + std::to_string(y);
        }

        Position directional_offset(const Direction& d) const {
            switch (d) {
                case Direction::NORTH:
                    return Position{x, (y == 0)?(constants::HEIGHT-1):(y-1)};
                case Direction::SOUTH:
                    return Position{x, (y == constants::HEIGHT-1)?0:(y+1)};
                case Direction::EAST:
                    return Position{(x == constants::WIDTH-1)?0:(x+1), y};
                case Direction::WEST:
                    return Position{(x == 0)?(constants::WIDTH-1):(x-1), y};
                case Direction::STILL:
                    return Position{x,y};
                default:
                    log::log(std::string("Error: directional_offset: unknown direction ") + static_cast<char>(d));
                    exit(1);
            }
        }

        void directional_offset_self(const Direction& d) {
            switch (d) {
                case Direction::NORTH:
                    y = (y == 0)?(constants::HEIGHT-1):(y-1);
                    break;
                case Direction::SOUTH:
                    y = (y == constants::HEIGHT-1)?0:(y+1);
                    break;
                case Direction::EAST:
                    x = (x == constants::WIDTH-1)?0:(x+1);
                    break;
                case Direction::WEST:
                    x= (x == 0)?(constants::WIDTH-1):(x-1);
                    break;
                case Direction::STILL:
                    break;
                default:
                    log::log(std::string("Error: directional_offset: unknown direction ") + static_cast<char>(d));
                    exit(1);
            }
        }
        void directional_offset(Position& position, const Direction& d) const {
            switch (d) {
                case Direction::NORTH:
                    position.y = (y == 0)?(constants::HEIGHT-1):(y-1);
                    position.x = x;
                    break;
                case Direction::SOUTH:
                    position.y = (y == constants::HEIGHT-1)?0:(y+1);
                    position.x = x;
                    break;
                case Direction::EAST:
                    position.y = y;
                    position.x = (x == constants::WIDTH-1)?0:(x+1);
                    break;
                case Direction::WEST:
                    position.y = y;
                    position.x = (x == 0)?(constants::WIDTH-1):(x-1);
                    break;
                case Direction::STILL:
                    break;
                default:
                    log::log(std::string("Error: directional_offset: unknown direction ") + static_cast<char>(d));
                    exit(1);
            }
        }
        std::array<Position, 4> get_surrounding_cardinals() {
            return {{
                directional_offset(Direction::NORTH), directional_offset(Direction::SOUTH),
                directional_offset(Direction::EAST), directional_offset(Direction::WEST)
            }};
        }
    };

    static std::ostream& operator<<(std::ostream& out, const Position& position) {
        out << position.x << ' ' << position.y;
        return out;
    }
    static std::istream& operator>>(std::istream& in, Position& position) {
        in >> position.x >> position.y;
        return in;
    }
}

namespace std {
    template <>
    struct hash<hlt::Position> {
        std::size_t operator()(const hlt::Position& position) const {
            return ((position.x+position.y) * (position.x+position.y+1) / 2) + position.y;
        }
    };
}

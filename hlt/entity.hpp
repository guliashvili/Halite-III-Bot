#pragma once

#include "position.hpp"
#include "types.hpp"

namespace hlt {
struct Entity {
  PlayerId owner;
  EntityId id; // if id is -1, then it's a shipyard
  Position position;

  Entity(PlayerId owner, EntityId id, int x, int y)
      : owner(owner), id(id), position(x, y) {}
};
} // namespace hlt

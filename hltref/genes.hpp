#pragma once

namespace hlt {
struct Genes {
  Genes(int argc, const char *argv[]);
  int seed, extra_time_for_recall, greedy_walk_randomisation_margin,
      margin_to_create_new_ship, total_halite_margin_substr,
      ship_spawn_step_margin, collision_caution_margin;
  double average_time_home_decay, dropoff_effect_decay_base;

private:
  template <class T> static T get_arg(T s, T e, const char *args);
};
} // namespace hlt

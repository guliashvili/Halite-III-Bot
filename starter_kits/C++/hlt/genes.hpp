#pragma once

namespace hlt {
    struct Genes {
      Genes(int argc, const char* argv[]);
      int seed,
      extra_time_for_recall,
      greedy_walk_randomisation_margin,
      margin_to_create_new_ship,
      total_halite_margin_substr,
      ship_spawn_step_margin,
      collision_caution_margin;
      double average_time_home_decay,
      average_efficiency_per_decay,
      average_halite_per_trip_decay;
    private:
      template<class T>
      static T get_arg(T s, T e, const char* args) ;
    };
}

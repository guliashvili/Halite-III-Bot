#include "genes.hpp"
#include "log.hpp"

#include <ctime>
#include <string>

using namespace std;

template <class T>
T hlt::Genes::get_arg(T s, T e, const char* args) {
  return static_cast<T>(stod(string(args)) * (e-s) + s);
}

hlt::Genes::Genes(int argc,  const char* argv[]){
    if(argc <= 1){
      seed = static_cast<int>(time(nullptr)%10000000);
      extra_time_for_recall = 5;
      greedy_walk_randomisation_margin = 1;
      margin_to_create_new_ship = 1000;
      total_halite_margin_substr = 3;
      average_time_home_decay = 1.0/3;
      ship_spawn_step_margin = 60;
      collision_caution_margin = 200;
      dropoff_effect_decay_base = 1.01;
    }else{
      seed = Genes::get_arg(0, 10000000, argv[1]);
      extra_time_for_recall = Genes::get_arg(0, 10, argv[2]);
      greedy_walk_randomisation_margin = Genes::get_arg(0, 50, argv[3]);
      margin_to_create_new_ship = Genes::get_arg(500, 2000, argv[4]);
      total_halite_margin_substr = Genes::get_arg(10, 100, argv[5]);
      average_time_home_decay = Genes::get_arg(double(0), double(1), argv[6]);
      ship_spawn_step_margin = Genes::get_arg(0, 200, argv[7]);
      collision_caution_margin = Genes::get_arg(0, 1000, argv[8]);
      dropoff_effect_decay_base =  Genes::get_arg(0, 1000, argv[9]);
    }


    log::log("seed: " + to_string(seed));
    log::log("extra_time_for_recall: " + to_string(extra_time_for_recall));
    log::log("greedy_walk_randomisation_margin: " + to_string(greedy_walk_randomisation_margin));
    log::log("margin_to_create_new_ship: " + to_string(margin_to_create_new_ship));
    log::log("total_halite_margin_substr: " + to_string(total_halite_margin_substr));
    log::log("average_time_home_decay: " + to_string(average_time_home_decay));
    log::log("ship_spawn_step_margin: " + to_string(ship_spawn_step_margin));

 }

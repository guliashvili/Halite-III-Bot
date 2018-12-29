#include "genes.hpp"
#include <ctime>
#include <string>

using namespace std;

template <class T>
T hlt::Genes::get_arg(T s, T e, char* args){
  return static_cast<T>(stod(string(args)) * (e-s) + s);
}

hlt::Genes::Genes(int argc,  char* argv[]){
    if(argc <= 1){
      seed = static_cast<int>(time(nullptr)%10000000);
      seed = 5;
      extra_time_for_recall = 5;
      traffic_controller_distance_margin = 5;
      greedy_walk_randomisation_margin = 1;
      margin_to_create_new_ship = 1000;
      total_halite_margin_substr = 24;
      average_time_home_decay = 1.0/3;
      ship_spawn_step_margin = 60;
    }else{
      seed = Genes::get_arg(0, 10000000, argv[1]);
      extra_time_for_recall = Genes::get_arg(0, 10, argv[2]);
      traffic_controller_distance_margin = Genes::get_arg(0, 10000000, argv[3]);
      greedy_walk_randomisation_margin = Genes::get_arg(0, 50, argv[4]);
      margin_to_create_new_ship = Genes::get_arg(500, 2000, argv[5]);
      total_halite_margin_substr = Genes::get_arg(10, 100, argv[6]);
      average_time_home_decay = Genes::get_arg(double(0), double(1), argv[7]);
      ship_spawn_step_margin = Genes::get_arg(0, 200, argv[8]);
    }

 }

#include "hlt/game.hpp"
#include "hlt/constants.hpp"
#include "hlt/log.hpp"
#include "hlt/genes.hpp"

#include <random>
#include <ctime>
#include <unordered_map>
#include <optional>
#include <tuple>
#include <array>
#include <algorithm>

using namespace std;
using namespace hlt;

unique_ptr<Genes> genes;
Game game;
shared_ptr<Player> me;

Command navigate(const shared_ptr<Ship> ship, const Direction& direction){
  game.game_map->navigate(ship, direction);
  return command::move(ship->id,direction);
}

Direction greedySquareMove(shared_ptr<Ship> ship, Position& target, bool recall=false){
  auto directions = game.game_map->get_safe_moves(ship->position, target, recall);
  if(directions.size() == 0){
    return Direction::STILL;
  }
  return directions[rand()%directions.size()];
}

pair<int, shared_ptr<Entity>> getMinDistanceToDropoff(Position& position, const vector<shared_ptr<Entity>>& dropoffs){
  pair<int, shared_ptr<Entity>>  minDistance;
  minDistance.first = 999999999;
  for(const auto& dropoff : dropoffs){
    int cur_distance = game.game_map->calculate_distance(position, dropoff->position);
    if(cur_distance < minDistance.first){
      minDistance.first = cur_distance;
      minDistance.second = dropoff;
    }
  }
  return minDistance;
}

std::optional<Direction> isRecallTime(shared_ptr<Ship> ship){
  auto minDstDropoff = getMinDistanceToDropoff(ship->position, me->all_dropoffs);
  if(constants::MAX_TURNS - game.turn_number - genes->extra_time_for_recall
    > minDstDropoff.first){
      return {};
  }
  return greedySquareMove(ship, minDstDropoff.second->position, true);
}


std::optional<Direction> shouldGoHome(shared_ptr<Ship> ship){
  if(ship->halite < constants::MAX_HALITE*9/10){
    return {};
  }

  auto minDstDropoff = getMinDistanceToDropoff(ship->position, me->all_dropoffs);
  return greedySquareMove(ship, minDstDropoff.second->position);
}


vector<tuple<unsigned,Position, double> > _candidates;
int ship_pair_st[64][64] = {{0}};
int MARK = 1;

void pair_ships(vector<shared_ptr<Ship> >& ships, vector<Command>& command_queue){
  auto& candidates = _candidates;
  candidates.resize(0);
  MARK++;

  {
    Position pos;
    for(int& y = pos.y = 0; y < constants::HEIGHT; y++){
        for(int& x = pos.x = 0; x < constants::WIDTH; x++){
            auto target_halite_amount = game.game_map->at(pos)->halite;
            if(target_halite_amount/constants::EXTRACT_RATIO == 0){
              continue;
            }

            for(unsigned i = 0; i < ships.size(); i++){
              auto ship = ships[i];
              int distance = game.game_map->calculate_distance(ship->position, pos);
              candidates.emplace_back(i, pos,
                min(constants::MAX_HALITE - ship->halite, target_halite_amount - 3) / double(distance+1));
            }
        }
    }
  }

  sort(candidates.begin(), candidates.end(), [ ]( const auto& lhs, const auto& rhs )
    {
       return std::get<2>(lhs) > std::get<2>(rhs);
    });

  for(auto& [ i, pos, efficiency ] : candidates){
    if(ships[i] == nullptr){
      continue;
    }
    if(ship_pair_st[pos.y][pos.x] == MARK){
      continue;
    }
    auto& ship = ships[i];
    if(pos != ship->position && game.game_map->get_safe_moves(ship->position, pos).size() == 0){
      continue;
    }

    command_queue.push_back(navigate(ship, greedySquareMove(ship, pos)));

    ship_pair_st[pos.y][pos.x] = MARK;
    ship = nullptr;
  }

}


bool GOING_HOME[1000] = {false};
int NUM_OF_MOVES_FROM_HOME[1000] = {0};
int AVERAGE_TIME_TO_HOME = 0;

bool should_ship_new_ship(){
  int total_me = game.me->ships.size();
  if(total_me == 0){
    return true;
  }

  int total_halite = 0;
  {
    Position position;
    for(int& i = position.y = 0; i < constants::HEIGHT; i++){
      for(int& j = position.x = 0; j < constants::WIDTH; j++){
        total_halite += game.game_map->at(position)->halite;
      }
    }
  }

  int total_ships_count = 0;
  for(const auto& player : game.players){
    total_ships_count += player->ships.size();
  }

  int current_halite_prediction = total_halite * total_me / total_ships_count;
  int next_halite_prediction = total_halite * (total_me + 1) / (total_ships_count + 1);

  return next_halite_prediction - current_halite_prediction > genes->margin_to_create_new_ship
    && constants::MAX_TURNS - AVERAGE_TIME_TO_HOME > genes->ship_spawn_step_margin;
}

void doStep(vector<Command>& command_queue){
  me = game.me;
  unique_ptr<GameMap>& game_map = game.game_map;
  game_map->init(me->id);
  int SAVINGS = 0;

  vector<shared_ptr<Ship>> ready_to_pair;
  for (const auto& ship : me->ships) {
      {
        auto cell = game_map->at(ship->position);
        if(cell->has_structure() && cell->structure->owner == me->id){
          GOING_HOME[ship->id] = false;
          AVERAGE_TIME_TO_HOME = AVERAGE_TIME_TO_HOME * genes->average_time_home_decay + NUM_OF_MOVES_FROM_HOME[ship->id] * (1-genes->average_time_home_decay);
          NUM_OF_MOVES_FROM_HOME[ship->id] = 0;
        }
      }

      if(!game_map->can_move(ship)){
        command_queue.push_back(ship->stay_still());
      }else if(auto recall=isRecallTime(ship)){
        command_queue.push_back(navigate(ship, recall.value()));
      }
      else if(auto goHome=shouldGoHome(ship)){
        command_queue.push_back(navigate(ship,goHome.value()));
      }else{
          ready_to_pair.push_back(ship);
      }
  }

  pair_ships(ready_to_pair, command_queue);

  if(me->halite - SAVINGS >= constants::SHIP_COST &&  !(game_map->at(me->shipyard->position)->is_occupied()) && should_ship_new_ship()){
    command_queue.push_back(me->shipyard->spawn());
  }
}

int main(int argc, char* argv[]) {

    _candidates.reserve(constants::WIDTH * constants::HEIGHT * 100);
    // At this point "game" variable is populated with initial map data.
    // This is a good place to do computationally expensive start-up pre-processing.
    // As soon as you call "ready" function below, the 2 second per turn timer will start.
    game.ready("MyCppBot");
    genes = make_unique<Genes>(argc, argv);
    srand(genes->seed);
    log::log("Successfully created bot! My Player ID is " + to_string(game.my_id) + ". Bot rng seed is " + to_string(genes->seed) + ".");

    vector<Command> command_queue;
    while(1){
      game.update_frame();

      command_queue.resize(0);
      command_queue.reserve(game.me->ships.size());
      doStep(command_queue);

      if (!game.end_turn(command_queue)) {
          break;
      }
    }






    return 0;
}

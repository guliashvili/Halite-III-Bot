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
#include <ctime>
#include <ratio>
#include <chrono>

using namespace std;
using namespace hlt;

shared_ptr<Genes> genes;
Game game;
shared_ptr<Player> me;

void navigate(const shared_ptr<Ship> ship, const Direction& direction,vector<tuple<shared_ptr<Ship>, Direction>>& direction_queue){
  game.game_map->navigate(ship, direction);
  direction_queue.emplace_back(ship, direction);
}

Direction greedySquareMove(shared_ptr<Ship> ship, Position& target, bool recall=false){
  auto directions = game.game_map->get_safe_moves(ship, target, recall);
  if(directions.size() == 0){
    return Direction::STILL;
  }else if(directions.size() == 1){
    return directions[0];
  }else {
    auto a = game.game_map->at(ship->position.directional_offset(directions[0]));
    auto b = game.game_map->at(ship->position.directional_offset(directions[1]));
    if(a->halite > b->halite){
      swap(a,b);
    }
    if(a->move_cost() > ship->halite - game.game_map->at(ship->position)->move_cost()){
      return directions[1];
    }else if(b->move_cost() - a->move_cost() < genes->greedy_walk_randomisation_margin){
      srand(ship->id * game.turn_number * target.x * target.y * genes->seed);
      return directions[rand()%directions.size()];
    }else{
      return directions[0];
    }
  }

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
  if(constants::MAX_TURNS - game.turn_number - genes->extra_time_for_recall > (minDstDropoff.first+me->ships.size()/10.0/me->all_dropoffs.size()) ){
      return {};
  }
  return greedySquareMove(ship, minDstDropoff.second->position, true);// TODO might stand still when blocked
}


bool shouldGoHome(shared_ptr<Ship> ship){
  return ship->halite >= constants::MAX_HALITE*9/10;
}



const int DP_MAX_TURNS = 100;
tuple<int, Direction, int> dp[DP_MAX_TURNS][64][64];

int DP_MARK = 1;


Position _dp_walk_next_pos;
vector<tuple<int, Direction, int> > compute_dp_walk(shared_ptr<Ship> ship, Position target, bool recall=false){
  DP_MARK++;

  // log::log(" Ship  " + to_string(ship->id) + " destination " + to_string(target.x) + " " + to_string(target.y));

  const auto moves_closer = game.game_map->get_unsafe_moves(ship->position, target);
  auto y_move = Direction::NORTH;
  auto x_move = Direction::EAST;
  if(find(moves_closer.begin(), moves_closer.end(), Direction::SOUTH) != moves_closer.end()){
    y_move = Direction::SOUTH;
  }
  if(find(moves_closer.begin(), moves_closer.end(), Direction::WEST) != moves_closer.end()){
    x_move = Direction::WEST;
  }

  dp[0][ship->position.x][ship->position.y] = {ship->halite, Direction::NONE, DP_MARK};

  const int MAX_CUR_TURN = (constants::WIDTH + constants::HEIGHT) * 1.3 / 2;
  for(int cur_turn = 0; cur_turn < MAX_CUR_TURN; cur_turn++){
    Position cur_edge_position = ship->position;
    while(1){

      Position cur_position = cur_edge_position;
      while(1){
        const auto& cur_dp_state = dp[cur_turn][cur_position.x][cur_position.y];
        if(get<2>(cur_dp_state) == DP_MARK){
            const vector<Direction>& moves = (cur_position == ship->position)?game.game_map->get_safe_moves(ship, target, recall):game.game_map->get_unsafe_moves(cur_position, target);
            // log::log(to_string(cur_turn) + " " + to_string(cur_position.x) + ":" + to_string(cur_position.y) + "  -  " + " hal " + to_string(get<0>(cur_dp_state)) + " move sizes " + to_string(moves.size()));

            for(auto move : moves){
              cur_position.directional_offset(_dp_walk_next_pos, move);
              int cur_halite = get<0>(cur_dp_state);
              int halite_to_grab = game.game_map->at(cur_position)->halite;

              int stay_turns = 0;


              while(1){
                const int halite_left = cur_halite - halite_to_grab / constants::MOVE_COST_RATIO;
                if(halite_left >= 0){
                  const int next_turn = cur_turn + stay_turns + 1;
                  if(next_turn < MAX_CUR_TURN){
                    if(get<2>(dp[next_turn][_dp_walk_next_pos.x][_dp_walk_next_pos.y]) != DP_MARK or get<0>(dp[next_turn][_dp_walk_next_pos.x][_dp_walk_next_pos.y]) < halite_left){
                          if(get<1>(cur_dp_state) == Direction::NONE){
                            // log::log("upd " + to_string(next_pos.x) + " " + to_string(next_pos.y) + " " + to_string(next_turn));
                            dp[next_turn][_dp_walk_next_pos.x][_dp_walk_next_pos.y] = {halite_left, (stay_turns>0)?Direction::STILL:move , DP_MARK};
                          }else{
                            // log::log("upd2 " + to_string(next_pos.x) + " " + to_string(next_pos.y) + " " + to_string(next_turn));
                            dp[next_turn][_dp_walk_next_pos.x][_dp_walk_next_pos.y] = {halite_left, get<1>(cur_dp_state), DP_MARK};
                          }

                    }
                  }
                }


                if(cur_halite == constants::MAX_HALITE or halite_to_grab/constants::EXTRACT_RATIO == 0){
                  break;
                }
                stay_turns++;
                const int extraction = (halite_to_grab+constants::EXTRACT_RATIO-1)/constants::EXTRACT_RATIO;
                cur_halite += extraction;
                cur_halite = min(cur_halite, constants::MAX_HALITE);
                halite_to_grab -= extraction;

              }

            }


        }


        if(cur_position.y == target.y){
          break;
        }
        cur_position.directional_offset_self(y_move);
      }

      if(cur_edge_position.x == target.x){
        break;
      }
      cur_edge_position.directional_offset_self(x_move);
    }
  }


  vector<tuple<int, Direction, int> > efficient_possibilities;
  for(int turn = 0; turn < MAX_CUR_TURN; turn++ ){
    if(get<2>(dp[turn][target.x][target.y]) == DP_MARK){
      efficient_possibilities.push_back({turn, get<1>(dp[turn][target.x][target.y]),get<0>(dp[turn][target.x][target.y])});
    }
  }

log::log("\n\n\n");
  return efficient_possibilities;


}

int NUM_OF_MOVES_FROM_HOME[1000] = {0};

Direction goToPointEfficient(shared_ptr<Ship> ship, Position destination){
  const auto dp_results = compute_dp_walk(ship, destination);
  if(dp_results.size() == 0){
    return Direction::STILL;
  }else{
    const int num_of_turns_from_home = NUM_OF_MOVES_FROM_HOME[ship->id];
    pair<double, Direction> best; best.first = -1;
    for(auto& [steps, direction, halite] : dp_results){
        double cur_efficiency = double(halite) / (num_of_turns_from_home + steps);
        if(cur_efficiency > get<0>(best)){
          best = {cur_efficiency, direction};
        }
    }
    return get<1>(best);
  }
}

Direction goToPointFast(shared_ptr<Ship> ship, Position destination, bool recall=false){
  if(ship->position == destination){
    return Direction::STILL;
  }

  const auto& dp_results = compute_dp_walk(ship, destination, recall);
  if(dp_results.size() == 0){
    return Direction::STILL;
  }else{
    return (get<1>(dp_results[0])==Direction::NONE)?Direction::STILL:get<1>(dp_results[0]);
  }
}

Direction goHome(shared_ptr<Ship> ship){
  auto minDstDropoff = getMinDistanceToDropoff(ship->position, me->all_dropoffs);

  return goToPointEfficient(ship, minDstDropoff.second->position);// TODO might stand still when blocked
}

vector<tuple<unsigned,Position, double> > _candidates;
int ship_pair_st[64][64] = {{0}};
int PAIR_MARK = 1;

void pair_ships(vector<shared_ptr<Ship> >& ships, vector<tuple<shared_ptr<Ship>, Direction>>&  ship_directions){
  PAIR_MARK++;
  auto& candidates = _candidates;
  candidates.resize(0);

  vector<tuple<shared_ptr<Ship>, Direction>> ret;

  {
    Position pos;
    for(int& y = pos.y = 0; y < constants::HEIGHT; y++){
        for(int& x = pos.x = 0; x < constants::WIDTH; x++){
            if(ship_pair_st[y][x] == PAIR_MARK){
              continue;
            }

            auto target_cell = game.game_map->at(pos);
            auto target_halite_amount = target_cell->halite;
            if(target_cell->extract_halite() == 0){
              continue;
            }
            //TODO SOMEONE ELSE SITS ON THAT

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

  for(unsigned I = 0; I < candidates.size(); I++){
    auto i = get<0>(candidates[I]);
    auto& pos = get<1>(candidates[I]);
    if(ships[i] == nullptr){
      continue;
    }
    if(ship_pair_st[pos.y][pos.x] == PAIR_MARK){
      continue;
    }
    auto& ship = ships[i];
    if(pos != ship->position && game.game_map->get_safe_moves(ship, pos).size() == 0){
      continue;
    }

    navigate(ship, goToPointFast(ship, pos), ship_directions);

    ship_pair_st[pos.y][pos.x] = PAIR_MARK;
    ship = nullptr;
  }

  for(auto& ship : ships){
    if(ship != nullptr){
      navigate(ship, Direction::STILL, ship_directions);
      //log::log("pairstill " + to_string(ship->id));
    }
  }
}


// bool GOING_HOME[1000] = {false};
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
        total_halite += max(0, game.game_map->at(position)->halite - genes->total_halite_margin_substr);
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

bool GO_HOME_EFFICIENT[1000] = {false};
vector<shared_ptr<Ship>> oplock_doStepNoStill(vector<shared_ptr<Ship> > ships, vector<tuple<shared_ptr<Ship>, Direction>>& direction_queue_original){
    vector<tuple<shared_ptr<Ship>, Direction>> direction_queue_temporary;
    direction_queue_temporary.reserve(ships.size());

    vector<shared_ptr<Ship>> ready_to_pair;
    vector<shared_ptr<Ship> > going_home;
    vector<shared_ptr<Ship> > going_home_now;
    sort(ships.begin(), ships.end(), [ ]( const auto& lhs, const auto& rhs )
      {
         return  get<0>(getMinDistanceToDropoff(lhs->position, game.me->all_dropoffs))
            < get<0>(getMinDistanceToDropoff(rhs->position, game.me->all_dropoffs));
      });
    for (const auto& ship : ships) {
        if(game.game_map->has_my_structure(ship->position)){
          GO_HOME_EFFICIENT[ship->id] = false;
        }

        if(auto recall=isRecallTime(ship)){
          //log::log("recalltime");
          // goHomeRecall(ship, recall.value())
          navigate(ship, recall.value(), direction_queue_temporary);
        }else if(GO_HOME_EFFICIENT[ship->id]){
          going_home.push_back(ship);
        }else if(shouldGoHome(ship)){
          going_home.push_back(ship);
          going_home_now.push_back(ship);
        }else{
          ready_to_pair.push_back(ship);
        }
    }

    bool was_blocker = std::find_if (direction_queue_temporary.begin(), direction_queue_temporary.end(), [] (const auto& element)
      {
        return get<1>(element) == Direction::STILL;
      }) != direction_queue_temporary.end();


    vector<shared_ptr<Ship>> ships_no_still;
    if(!was_blocker){
      pair_ships(ready_to_pair, direction_queue_temporary);

      for(auto& ship : going_home){
        const auto& goHomeDir=goHome(ship);
        navigate(ship, goHomeDir, direction_queue_temporary);
        GO_HOME_EFFICIENT[ship->id] = true;
      }

    }else{
      for(auto& ship : ready_to_pair){
        ships_no_still.push_back(ship);
      }
      for(auto& ship : going_home){
        ships_no_still.push_back(ship);
      }
    }


    was_blocker = was_blocker || std::find_if (direction_queue_temporary.begin(), direction_queue_temporary.end(), [] (const auto& element)
      {
        return get<1>(element) == Direction::STILL;
      }) != direction_queue_temporary.end();

    if(was_blocker){
      ships_no_still.reserve(ships.size());
      for(auto& [ship, direction] : direction_queue_temporary){
        if(direction != Direction::STILL){
          auto target_pos = ship->position.directional_offset(direction);
          if(game.game_map->at(target_pos)->ship == ship){
            game.game_map->at(target_pos)->mark_safe();
          }
          ships_no_still.push_back(ship);
        }else{
          game.game_map->at(ship->position)->mark_unsafe(ship);
          direction_queue_original.emplace_back(ship, direction);
          // log::log("stay stil; " + to_string(ship->id));
        }
      }

      for(auto ship : going_home_now){
        GO_HOME_EFFICIENT[ship->id] = false;
      }

      return ships_no_still;
    }else{
      // for(auto& [ship, direction] : direction_queue_temporary){
      //   log::log("the end: " + to_string(ship->id));
      // }
      direction_queue_original.insert(direction_queue_original.end(), direction_queue_temporary.begin(), direction_queue_temporary.end());
      return {};
    }
}

bool doStep(vector<tuple<shared_ptr<Ship>, Direction>>& direction_queue){
  using namespace std::chrono;

  // high_resolution_clock::time_point t1 = high_resolution_clock::now();

  me = game.me;
  unique_ptr<GameMap>& game_map = game.game_map;
  game_map->init(me->id, genes);

  int SAVINGS = 0;

  vector<shared_ptr<Ship> > ships;

  for (const auto& ship : me->ships) {
      if(game_map->has_my_structure(ship->position)){
        // GOING_HOME[ship->id] = false;
        AVERAGE_TIME_TO_HOME = AVERAGE_TIME_TO_HOME * genes->average_time_home_decay + NUM_OF_MOVES_FROM_HOME[ship->id] * (1-genes->average_time_home_decay);
        NUM_OF_MOVES_FROM_HOME[ship->id] = 0;
      }
      if(!game_map->can_move(ship)){
        //log::log("can not move " + to_string(ship->id));
        navigate(ship, Direction::STILL, direction_queue);
      }else{
        game.game_map->at(ship->position)->mark_safe();
        ships.push_back(ship);
      }
  }

  // int num_of_turns = 0;
  while((ships=oplock_doStepNoStill(ships, direction_queue)).size() != 0){
    // num_of_turns++;
    //log::log("\n\n");
  }


  for (const auto& ship : me->ships) {
      NUM_OF_MOVES_FROM_HOME[ship->id]++;
  }

  //log::log("doStep duration: " + to_string(duration_cast<duration<double>>(high_resolution_clock::now() - t1).count()) + " num: " + to_string(num_of_turns)  + " ships: " + to_string(me->ships.size()));
  auto ret = me->halite - SAVINGS >= constants::SHIP_COST &&  !(game_map->at(me->shipyard->position)->is_occupied()) && should_ship_new_ship();
  return ret;
}

int main(int argc, const char* argv[]) {

    _candidates.reserve(constants::WIDTH * constants::HEIGHT * 100);
    // At this point "game" variable is populated with initial map data.
    // This is a good place to do computationally expensive start-up pre-processing.
    // As soon as you call "ready" function below, the 2 second per turn timer will start.
    game.ready("MyCppBot");
    genes = make_shared<Genes>(argc, argv);
    srand(genes->seed);
    //log::log("Successfully created bot! My Player ID is " + to_string(game.my_id) + ". Bot rng seed is " + to_string(genes->seed) + ".");

    vector<Command> command_queue;
    vector<tuple<shared_ptr<Ship>, Direction>> direction_queue;
    while(1){
      game.update_frame();

      command_queue.resize(0);
      command_queue.reserve(game.me->ships.size() + 1);
      direction_queue.resize(0);
      direction_queue.reserve(game.me->ships.size());


      if(doStep(direction_queue)){
        command_queue.push_back(command::spawn_ship());
      }

      for(auto& [ship, direction] : direction_queue){
        command_queue.push_back(command::move(ship->id, direction));
      }

      if (!game.end_turn(command_queue)) {
          break;
      }
    }

    return 0;
}

/*****
 *
 *
 *  DO NOT EDIT - change MyBot.cpp instead
 *  This is reference bot which is performing well so any changes
 *  should be compared against it.
 *
 *  When improved - just replace this code with new reference bot
 *
 *
*****/

#include "hltref/constants.hpp"
#include "hltref/game.hpp"
#include "hltref/genes.hpp"
#include "hltref/log.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <ctime>
#include <optional>
#include <random>
#include <ratio>
#include <tuple>
#include <unordered_map>
#include <cmath>
#include <set>

using namespace std;
using namespace hlt;
using namespace std::chrono;

shared_ptr<Genes> genes;
Game game;
shared_ptr<Player> me;
high_resolution_clock::time_point t1;

// Opponent analytics
vector<Position> our_dropoffs;
vector<Position> our_ships;
vector<Position> opponents_dropoffs;
vector<Position> opponents_ships;
int distance_from_our_dropoffs[64][64];
int distance_from_our_ships[64][64];
int distance_from_their_dropoffs[64][64];
int distance_from_their_ships[64][64];
int probability_of_invasion[64][64];

shared_ptr<Ship> analytics_ship_last_state[1000];
shared_ptr<Ship> analytics_ship_cur_state[1000];
vector<Direction> analytics_ship_directions[1000];
vector<Direction> analytics_ship_will_go_to[1000];
int analytics_total_halite = 0;

int adjust(int value, int max) {
  return (value + max) % max;
}

void bfs(int (&d)[64][64], vector<Position> &q) {
  memset(d, -1, sizeof d);
  for(int i=0; i<q.size(); i++){
    d[q[i].x][q[i].y] = 0;
  }
  int xx[] = { -1, 0, 1, 0 };
  int yy[] = { 0, 1, 0, -1 };
  for(int i=0; i<q.size(); i++){
    for (int j=0; j<4; j++) {
      int x = adjust(q[i].x + xx[j], constants::WIDTH);
      int y = adjust(q[i].y + yy[j], constants::HEIGHT);

      if (d[x][y] == -1) {
        d[x][y] = d[q[i].x][q[i].y] + 1;
        q.push_back(Position(x, y));
      }
    }
  }
}

void computeProbabilityOfInvasion() {
  bfs(distance_from_our_dropoffs, our_dropoffs);
  bfs(distance_from_our_ships, our_ships);
  bfs(distance_from_their_dropoffs, opponents_dropoffs);
  bfs(distance_from_their_ships, opponents_ships);

  memset(probability_of_invasion, 0, sizeof probability_of_invasion);
  for (int y = 0; y < constants::HEIGHT; y++) {
    for (int x = 0; x < constants::WIDTH; x++) {
      int v = distance_from_our_ships[x][y] - distance_from_their_ships[x][y];
      probability_of_invasion[x][y] = (int)(100.0 * (v + constants::HEIGHT) / 2 / constants::HEIGHT);
    }
  }
}

void updatePositionsOfOpponentsStuff() {
  analytics_total_halite = 0;
  {
    Position position;
    for (int &i = position.y = 0; i < constants::HEIGHT; i++) {
      for (int &j = position.x = 0; j < constants::WIDTH; j++) {
        analytics_total_halite += max(0, game.game_map->at(position)->halite -
                                   genes->total_halite_margin_substr);
      }
    }
  }

  our_dropoffs.clear();
  our_ships.clear();
  opponents_dropoffs.clear();
  opponents_ships.clear();
  for (auto player : game.players) {
    for (auto dropoff : player->all_dropoffs) {
      if(player->id != game.me->id){
        opponents_dropoffs.push_back(dropoff->position);
      } else {
        our_dropoffs.push_back(dropoff->position);
      }
    }
    for (auto ship : player->ships) {
      if(player->id != game.me->id){
        opponents_ships.push_back(ship->position);
      } else {
        our_ships.push_back(ship->position);
      }

      analytics_ship_last_state[ship->id] = analytics_ship_cur_state[ship->id];
      analytics_ship_cur_state[ship->id] = ship;
      if(analytics_ship_last_state[ship->id] != nullptr){
        for(auto direction : ALL_CARDINALS){
          if(ship->position == analytics_ship_last_state[ship->id]->position.directional_offset(direction)){
            analytics_ship_directions[ship->id].push_back(direction);
            break;
          }
        }
      }

      analytics_ship_will_go_to[ship->id] = {Direction::NORTH, Direction::SOUTH, Direction::WEST, Direction::EAST, Direction::STILL};

      if(analytics_ship_directions[ship->id].size() > 5){
        analytics_ship_directions[ship->id] = vector<Direction>(analytics_ship_directions[ship->id].end() - 5, analytics_ship_directions[ship->id].end());
        set<Direction> directions_used(analytics_ship_directions[ship->id].begin(), analytics_ship_directions[ship->id].end());
        if(directions_used.count(Direction::STILL) == 0 && directions_used.size() <= 2){

          vector<Direction> will_not_go_to;
          analytics_ship_will_go_to[ship->id] = {Direction::STILL}; //TODO will it really STILL?

          for(Direction direction_used : directions_used){
            will_not_go_to.push_back(invert_direction(direction_used));
          }
          for(auto direction : ALL_CARDINALS){
            if(find(will_not_go_to.begin(), will_not_go_to.end(), direction) == will_not_go_to.end()){
              analytics_ship_will_go_to[ship->id].push_back(direction);
            }
          }


        }
      }
    }
  }

  computeProbabilityOfInvasion();
}

// [1.0, 2 ^ (PLAYERS-1)] - impact of opponents dropoffs on the area
double getDropoffImpact(Position pos, Position dropoff){
  const int num_of_players = game.players.size();
  int distance = game.game_map->calculate_distance(pos, dropoff);
  return 2 - pow(genes->dropoff_effect_decay_base, distance);
}

double impactOfOpponentsDropoffs(Position pos) {
  double impact = 1.0;
  for (auto dropoff : opponents_dropoffs) {
    impact *= getDropoffImpact(pos, dropoff);
  }
  return impact;
}

int distance_from_our_other_dropoffs(Position ref_dropoff, Position pos) {
  int result = constants::HEIGHT + constants::WIDTH;
  for (auto dropoff : game.me->all_dropoffs) {
    if (dropoff->position == ref_dropoff) {
      continue;
    } else if (result > game.game_map->calculate_distance(pos, dropoff->position)) {
      result = game.game_map->calculate_distance(pos, dropoff->position);
    }
  }
  return result;
}

// end opponent analytics

int get_milisecond_left() {
  return 2000 - duration_cast<std::chrono::milliseconds>(
                    high_resolution_clock::now() - t1)
                    .count();
}

void navigate(const shared_ptr<Ship> ship, const Direction &direction,
              vector<tuple<shared_ptr<Ship>, Direction>> &direction_queue) {
  game.game_map->navigate(ship, direction);
  direction_queue.emplace_back(ship, direction);
}

Direction greedySquareMove(shared_ptr<Ship> ship, Position &target,
                           bool recall = false) {
  auto directions = game.game_map->get_safe_moves(ship, target, recall);
  if (directions.size() == 0) {
    return Direction::STILL;
  } else if (directions.size() == 1) {
    return directions[0];
  } else {
    auto a =
        game.game_map->at(ship->position.directional_offset(directions[0]));
    auto b =
        game.game_map->at(ship->position.directional_offset(directions[1]));
    if (a->halite > b->halite) {
      swap(a, b);
      swap(directions[0], directions[1]);
    }
    if (a->move_cost() >
        ship->halite - game.game_map->at(ship->position)->move_cost()) {
      return directions[1];
    } else if (b->move_cost() - a->move_cost() <
               genes->greedy_walk_randomisation_margin) {
      srand(ship->id * game.turn_number * target.x * target.y * genes->seed);
      return directions[rand() % directions.size()];
    } else {
      return directions[0];
    }
  }
}


bool faking_dropoff[64][64];

pair<int, shared_ptr<Entity>>
getMinDistanceToDropoff(Position &position,
                        const vector<shared_ptr<Entity>> &dropoffs) {
  pair<int, shared_ptr<Entity>> minDistance;
  minDistance.first = 999999999;
  for (const auto &dropoff : dropoffs) {
    if(faking_dropoff[dropoff->position.x][dropoff->position.y] && game.me->halite < 3000){
      continue;
    }
    int cur_distance =
        game.game_map->calculate_distance(position, dropoff->position);
    if (cur_distance < minDistance.first) {
      minDistance.first = cur_distance;
      minDistance.second = dropoff;
    }
  }
  return minDistance;
}

std::optional<Direction> isRecallTime(shared_ptr<Ship> ship) {
  auto minDstDropoff =
      getMinDistanceToDropoff(ship->position, me->all_dropoffs);
  if (constants::MAX_TURNS - game.turn_number - genes->extra_time_for_recall >
      (minDstDropoff.first +
       me->ships.size() / 10.0 / me->all_dropoffs.size())) {
    return {};
  }
  return greedySquareMove(ship, minDstDropoff.second->position,
                          true); // TODO might stand still when blocked
}

bool shouldGoHome(shared_ptr<Ship> ship) {
  // Heuristic: return home with halite ship if collected >= 900
  return ship->halite >= constants::MAX_HALITE * 9 / 10;
}

const int DP_MAX_TURNS = 3*64;
tuple<int, Direction, int> dp[64][64][DP_MAX_TURNS];

int DP_MARK = 1;

vector<tuple<int, Direction, int>>
compute_dp_walk(shared_ptr<Ship> ship, Position target, bool recall = false) {
  if (get_milisecond_left() < 10*game.me->ships.size()) {
    return {{0, greedySquareMove(ship, target, recall), 0}};
  }

  DP_MARK++;

  const auto moves_closer =
      game.game_map->get_unsafe_moves(ship->position, target);
  auto y_move = Direction::NORTH;
  auto x_move = Direction::EAST;
  if (find(moves_closer.begin(), moves_closer.end(), Direction::SOUTH) !=
      moves_closer.end()) {
    y_move = Direction::SOUTH;
  }
  if (find(moves_closer.begin(), moves_closer.end(), Direction::WEST) !=
      moves_closer.end()) {
    x_move = Direction::WEST;
  }

  dp[ship->position.x][ship->position.y][0] = {ship->halite, Direction::NONE,
                                               DP_MARK};

  const int MAX_CUR_TURN = 2 * game.game_map->calculate_distance(ship->position, target) + 6;

  Position cur_edge_position = ship->position;
  int edge_dist = 0;
  while (1) {

    Position cur_position = cur_edge_position;
    int cur_dist = edge_dist;
    while (1) {
      bool found_some = false;
      const vector<Direction> &moves =
          (cur_position == ship->position)
              ? game.game_map->get_safe_moves(ship, target, recall)
              : game.game_map->get_unsafe_moves(cur_position, target);
      vector<pair<Direction, Position>> move_pos;
      move_pos.reserve(moves.size());
      for (auto move : moves) {
        move_pos.emplace_back(move, cur_position.directional_offset(move));
      }
      int upper_turn_bound = MAX_CUR_TURN - game.game_map->calculate_distance(cur_position, target) + 1;

      for (int cur_turn = cur_dist; cur_turn < upper_turn_bound; cur_turn++) {
        const auto &cur_dp_state = dp[cur_position.x][cur_position.y][cur_turn];

        if (get<2>(cur_dp_state) == DP_MARK) {
          found_some = true;
          // log::log(to_string(cur_turn) + " " + to_string(cur_position.x) +
          // ":" + to_string(cur_position.y) + "  -  " + " hal " +
          // to_string(get<0>(cur_dp_state)) + " move sizes " +
          // to_string(moves.size()));

          for (const auto &[move, _dp_walk_next_pos] : move_pos) {
            int halite_to_grab = game.game_map->at(cur_position)->halite;
            int cur_halite = get<0>(cur_dp_state);
            int stay_turns = 0;

            while (1) {
              const int halite_left =
                  cur_halite - halite_to_grab / constants::MOVE_COST_RATIO;
              if (halite_left >= 0) {
                const int next_turn = cur_turn + stay_turns + 1;
                if (next_turn < MAX_CUR_TURN) {
                  if (get<2>(dp[_dp_walk_next_pos.x][_dp_walk_next_pos.y]
                               [next_turn]) != DP_MARK ||
                      get<0>(dp[_dp_walk_next_pos.x][_dp_walk_next_pos.y]
                               [next_turn]) < halite_left) {
                    if (get<1>(cur_dp_state) == Direction::NONE) {
                      // log::log("upd " + to_string(next_pos.x) + " " +
                      // to_string(next_pos.y) + " " + to_string(next_turn));
                      dp[_dp_walk_next_pos.x][_dp_walk_next_pos.y][next_turn] =
                          {halite_left,
                           (stay_turns > 0) ? Direction::STILL : move, DP_MARK};
                    } else {
                      // log::log("upd2 " + to_string(next_pos.x) + " " +
                      // to_string(next_pos.y) + " " + to_string(next_turn));
                      dp[_dp_walk_next_pos.x][_dp_walk_next_pos.y][next_turn] =
                          {halite_left, get<1>(cur_dp_state), DP_MARK};
                    }
                  }
                }
              }

              if (cur_halite == constants::MAX_HALITE ||
                  halite_to_grab / constants::EXTRACT_RATIO == 0) {
                break;
              }
              stay_turns++;
              const int extraction =
                  (halite_to_grab + constants::EXTRACT_RATIO - 1) /
                  constants::EXTRACT_RATIO;
              cur_halite += extraction;
              cur_halite = min(cur_halite, constants::MAX_HALITE);
              halite_to_grab -= extraction;
            }
          }

        } else {
          if (found_some) {
            break;
          }
        }
      }

      if (cur_position.y == target.y) {
        break;
      }
      cur_position.directional_offset_self(y_move);
      cur_dist++;
    }

    if (cur_edge_position.x == target.x) {
      break;
    }
    cur_edge_position.directional_offset_self(x_move);
    edge_dist++;
  }

  vector<tuple<int, Direction, int>> efficient_possibilities;
  for (int turn = 0; turn < MAX_CUR_TURN; turn++) {
    if (get<2>(dp[target.x][target.y][turn]) == DP_MARK) {
      efficient_possibilities.push_back({turn,
                                         (get<1>(dp[target.x][target.y][turn])==Direction::NONE)?Direction::STILL:get<1>(dp[target.x][target.y][turn]),
                                         get<0>(dp[target.x][target.y][turn])});
    }
  }
  return efficient_possibilities;
}

int NUM_OF_MOVES_FROM_HOME[1000] = {0};

Direction goToPointEfficient(shared_ptr<Ship> ship, Position destination) {
  const auto dp_results = compute_dp_walk(ship, destination);
  if (dp_results.size() == 0) {
    return Direction::STILL;
  } else {
    const int num_of_turns_from_home = NUM_OF_MOVES_FROM_HOME[ship->id];
    pair<double, Direction> best = {-1, Direction::STILL};
    for (auto &[steps, direction, halite] : dp_results) {
      double cur_efficiency = double(halite) / (num_of_turns_from_home + steps);
      if (cur_efficiency > get<0>(best)) {
        best = {cur_efficiency, direction};
      }
    }
    return get<1>(best);
  }
}

Direction goToPointFast(shared_ptr<Ship> ship, Position destination,
                        bool recall = false) {
  if (ship->position == destination) {
    return Direction::STILL;
  }

  const auto &dp_results = compute_dp_walk(ship, destination, recall);
  if (dp_results.size() == 0) {
    return Direction::STILL;
  } else {
    return get<1>(dp_results[0]);
  }
}

Direction goHome(shared_ptr<Ship> ship) {
  auto minDstDropoff =
      getMinDistanceToDropoff(ship->position, me->all_dropoffs);

  return goToPointEfficient(
      ship,
      minDstDropoff.second->position); // TODO might stand still when blocked
}

vector<tuple<unsigned, Position, double>> _candidates;
int ship_pair_st[64][64] = {{0}};
int PAIR_MARK = 1;

void pair_ships(vector<shared_ptr<Ship>> &ships,
                vector<tuple<shared_ptr<Ship>, Direction>> &ship_directions) {
  PAIR_MARK++;
  auto &candidates = _candidates;
  candidates.resize(0);

  vector<tuple<shared_ptr<Ship>, Direction>> ret;

  {
    Position pos;
    for (int &y = pos.y = 0; y < constants::HEIGHT; y++) {
      for (int &x = pos.x = 0; x < constants::WIDTH; x++) {
        if (ship_pair_st[y][x] == PAIR_MARK) {
          continue;
        }

        auto target_cell = game.game_map->at(pos);
        auto target_halite_amount = target_cell->halite;
        if (target_cell->extract_halite() == 0) {
          continue;
        }
        // TODO SOMEONE ELSE SITS ON THAT

        for (unsigned i = 0; i < ships.size(); i++) {
          auto ship = ships[i];
          int distance = game.game_map->calculate_distance(ship->position, pos);
          candidates.emplace_back(i, pos,
                                  min(constants::MAX_HALITE - ship->halite,
                                      target_halite_amount - 3) /
                                      double(distance + 1));
        }
      }
    }
  }

  sort(candidates.begin(), candidates.end(),
       [](const auto &lhs, const auto &rhs) {
         return std::get<2>(lhs) > std::get<2>(rhs);
       });

  for (unsigned I = 0; I < candidates.size(); I++) {
    auto i = get<0>(candidates[I]);
    auto &pos = get<1>(candidates[I]);
    if (ships[i] == nullptr) {
      continue;
    }
    if (ship_pair_st[pos.y][pos.x] == PAIR_MARK) {
      continue;
    }
    auto &ship = ships[i];
    if (pos != ship->position &&
        game.game_map->get_safe_moves(ship, pos).size() == 0) {
      continue;
    }

    navigate(ship, goToPointFast(ship, pos), ship_directions);

    ship_pair_st[pos.y][pos.x] = PAIR_MARK;
    ship = nullptr;
  }

  for (auto &ship : ships) {
    if (ship != nullptr) {
      navigate(ship, Direction::STILL, ship_directions);
      // log::log("pairstill " + to_string(ship->id));
    }
  }
}

int AVERAGE_TIME_TO_HOME = 0;

bool should_ship_new_ship() {
  int total_me = game.me->ships.size();
  if (total_me == 0) {
    return true;
  }

  int total_ships_count = 0;
  for (const auto &player : game.players) {
    total_ships_count += player->ships.size();
  }

  int current_halite_prediction = analytics_total_halite * total_me / total_ships_count;
  int next_halite_prediction =
      analytics_total_halite * (total_me + 1) / (total_ships_count + 1);

  return next_halite_prediction - current_halite_prediction >
             genes->margin_to_create_new_ship &&
         constants::MAX_TURNS - AVERAGE_TIME_TO_HOME >
             genes->ship_spawn_step_margin;
}


// Dropoff
vector<double> dropOff_potential[64][64];
void updateDropoffCandidates() {
  Position pos;
  for(int &x = pos.x = 0; x < constants::WIDTH; x++){
    for(int &y = pos.y = 0; y < constants::HEIGHT; y++){
      const int effect_distance = constants::WIDTH / game.players.size() / 2;

      vector<double> &halite_quality = dropOff_potential[x][y];
      halite_quality.clear();
      for(int delta_x = -effect_distance; delta_x < effect_distance; delta_x++){
        for(int delta_y = -(effect_distance-abs(delta_x)); delta_y < (effect_distance-abs(delta_x)); delta_y++){
          Position cur_cell(adjust(x+delta_x, constants::WIDTH), adjust(y+delta_y, constants::HEIGHT));

          int h = game.game_map->at(cur_cell)->halite;

          double optimistic_score = (double) h / 2 / (delta_x + delta_y + 1);
          optimistic_score -= (double) h / 2 / (distance_from_our_other_dropoffs(pos, cur_cell) + 1);
          double realistic_score = (double) optimistic_score * (100 - probability_of_invasion[cur_cell.x][cur_cell.y]);
          halite_quality.push_back(realistic_score);
        }
      }
      sort(halite_quality.begin(), halite_quality.end(), [](auto l, auto r) { return l < r; });
    }
  }
}

vector<Position> faking_dropoffs;

optional<Position> shallWeInvestInDropOff() {
  if (!faking_dropoffs.empty()) {
    return {};
  }
  int ships = game.me->ships.size();

  vector<double> existing_potential;
  for (auto d : game.me->all_dropoffs) {
    existing_potential.insert(
      existing_potential.end(),
      dropOff_potential[d->position.x][d->position.y].begin(),
      dropOff_potential[d->position.x][d->position.y].end()
    );
  }
  sort(existing_potential.begin(), existing_potential.end(), [](auto l, auto r) { return l < r; });

  double combined_profit[10000];
  for(int i=0; i<10000; i++){
    combined_profit[i] = (i == 0) ? 0 : combined_profit[i-1];
    if (i < existing_potential.size()) {
      combined_profit[i] += existing_potential[i];
    }
  }

  pair<double, Position> best_dropoff;

  Position pos;
  for(int &x = pos.x = 0; x < constants::WIDTH; x++){
    for(int &y = pos.y = 0; y < constants::HEIGHT; y++){
      if(game.game_map->at(pos)->has_structure() || faking_dropoff[pos.x][pos.y]){
        continue;
      }
      int d = distance_from_our_ships[pos.x][pos.y];
      double initial_cost = -constants::DROPOFF_COST
        -constants::SHIP_COST
        - (combined_profit[d * ships] - combined_profit[d * (ships - 1)]);

      // amount of harite mined at new dropoff vs all other combined (if no dropoff would be built)
      int ships_assigned_to_new_dropoff = (int) (ships + game.me->all_dropoffs.size()) / (game.me->all_dropoffs.size() + 1);
      int ships_left = ships - ships_assigned_to_new_dropoff;
      int start_point = d * (ships - 1);
      int target_round = constants::HEIGHT / game.players.size() / 4;
      double profit = initial_cost -
        (combined_profit[target_round * ships] - combined_profit[start_point + (target_round - d - 1) * ships_left]);
      for (int round = d + 1, i=0; round < target_round && i < dropOff_potential[x][y].size(); round++) {
        for(int j=0; j<ships_assigned_to_new_dropoff && i < dropOff_potential[x][y].size(); j++, i++) {
          profit += dropOff_potential[x][y][i];
        }
      }
      if(profit > get<0>(best_dropoff)){
        best_dropoff = {profit, pos};
      }
    }
  }

  if(get<0>(best_dropoff) > 0.0){
    return get<1>(best_dropoff);
  } else {
    return {};
  }
}

// bool isTimeToDropoff(){
//   return game.turn_number > 50 && game.me->ships.size() > 20 && (faking_dropoffs.size() + game.me->dropoffs.size()) == 0;
// }
//
// Position find_dropoff_place(){
//
//   pair<double, Position> best_dropoff;
//   best_dropoff.first = -1;
//   Position pos;
//   for(int &x = pos.x = 0; x < constants::WIDTH; x++){
//     for(int &y = pos.y = 0; y < constants::HEIGHT; y++){
//       if(game.game_map->at(pos)->has_structure() || faking_dropoff[pos.x][pos.y]){
//         continue;
//       }
//       const int effect_distance = constants::WIDTH / game.players.size() / 4;
//
//       double total_halite_in_range = 0;
//       for(int delta_x = -effect_distance; delta_x < effect_distance; delta_x++){
//         for(int delta_y = -(effect_distance-abs(delta_x)); delta_y < (effect_distance-abs(delta_x)); delta_y++){
//           Position cur_cell( (((x+delta_x)%constants::WIDTH)+constants::WIDTH)%constants::WIDTH,  (((y+delta_y)%constants::HEIGHT)+constants::HEIGHT)%constants::HEIGHT);
//           total_halite_in_range += game.game_map->at(cur_cell)->halite;
//         }
//       }
//
//       total_halite_in_range /= impactOfOpponentsDropoffs(pos);
//
//       if(total_halite_in_range > get<0>(best_dropoff)){
//         best_dropoff = {total_halite_in_range, pos};
//       }
//     }
//   }
//
//   return get<1>(best_dropoff);
// }
// Dropoff - End

// boolean flag telling is ship currently going home (to shipyard)
bool GO_HOME_EFFICIENT[1000] = {false};

vector<shared_ptr<Ship>> oplock_doStepNoStill(
    vector<shared_ptr<Ship>> ships,
    vector<tuple<shared_ptr<Ship>, Direction>> &direction_queue_original) {
  vector<tuple<shared_ptr<Ship>, Direction>> direction_queue_temporary;
  direction_queue_temporary.reserve(ships.size());

  vector<shared_ptr<Ship>> ready_to_pair;
  vector<shared_ptr<Ship>> going_home;
  vector<shared_ptr<Ship>> going_home_now;
  sort(ships.begin(), ships.end(), [](const auto &lhs, const auto &rhs) {
    return get<0>(
               getMinDistanceToDropoff(lhs->position, game.me->all_dropoffs)) <
           get<0>(
               getMinDistanceToDropoff(rhs->position, game.me->all_dropoffs));
  });
  for (const auto &ship : ships) {

    // returned to home shipyard
    if (game.game_map->has_my_structure(ship->position)) {
      GO_HOME_EFFICIENT[ship->id] = false;
    }

    // time to return home at the end of the game
    if (auto recall = isRecallTime(ship)) {
      navigate(ship, recall.value(), direction_queue_temporary);
    } else if (GO_HOME_EFFICIENT[ship->id]) {
      going_home.push_back(ship);
    } else if (shouldGoHome(ship)) {
      going_home.push_back(ship);
      going_home_now.push_back(ship);
    } else {
      ready_to_pair.push_back(ship);
    }
  }

  bool was_blocker =
      std::find_if(direction_queue_temporary.begin(),
                   direction_queue_temporary.end(), [](const auto &element) {
                     return get<1>(element) == Direction::STILL;
                   }) != direction_queue_temporary.end();

  vector<shared_ptr<Ship>> ships_no_still;
  if (!was_blocker) {
    pair_ships(ready_to_pair, direction_queue_temporary);

    for (auto &ship : going_home) {
      const auto &goHomeDir = goHome(ship);
      navigate(ship, goHomeDir, direction_queue_temporary);
      GO_HOME_EFFICIENT[ship->id] = true;
    }

  } else {
    for (auto &ship : ready_to_pair) {
      ships_no_still.push_back(ship);
    }
    for (auto &ship : going_home) {
      ships_no_still.push_back(ship);
    }

  }
  was_blocker = was_blocker ||
      std::find_if(direction_queue_temporary.begin(),
                   direction_queue_temporary.end(), [](const auto &element) {
                     return get<1>(element) == Direction::STILL && game.game_map->at(get<0>(element)->position)->collision;;
                   }) != direction_queue_temporary.end();
  if (was_blocker) {
    ships_no_still.reserve(ships.size());
    for (auto &[ship, direction] : direction_queue_temporary) {
      if (direction != Direction::STILL) {
        auto target_pos = ship->position.directional_offset(direction);
        if (game.game_map->at(target_pos)->ship == ship) {
          game.game_map->at(target_pos)->mark_safe();
        }
        ships_no_still.push_back(ship);
      } else {
        game.game_map->at(ship->position)->mark_unsafe(ship);
        direction_queue_original.emplace_back(ship, direction);
      }
    }

    for (auto ship : going_home_now) {
      GO_HOME_EFFICIENT[ship->id] = false;
    }

    return ships_no_still;
  } else {
    // for(auto& [ship, direction] : direction_queue_temporary){
    //   log::log("the end: " + to_string(ship->id));
    // }
    direction_queue_original.insert(direction_queue_original.end(),
                                    direction_queue_temporary.begin(),
                                    direction_queue_temporary.end());
    return {};
  }
}

int savings = 0;
vector<Command> doStep(vector<tuple<shared_ptr<Ship>, Direction>> &direction_queue) {

  t1 = high_resolution_clock::now();
  vector<Command> constructions;
  me = game.me;
  unique_ptr<GameMap> &game_map = game.game_map;
  game_map->init(me->id, genes, analytics_ship_will_go_to);

  updatePositionsOfOpponentsStuff();
  updateDropoffCandidates();

  auto dropoff_assessment = shallWeInvestInDropOff();

  if(dropoff_assessment) {
    savings += constants::DROPOFF_COST;
    Position pos = *dropoff_assessment;
    faking_dropoff[pos.x][pos.y] = true;
    faking_dropoffs.push_back(pos);
  }

  vector<Position> abandoned_dropoffs;
  for(auto &position : faking_dropoffs){
    if(game.game_map->at(position)->has_structure() && game.game_map->at(position)->structure->owner != me->id){
      faking_dropoff[position.x][position.y] = false;
      auto dropoff_assessment = shallWeInvestInDropOff();
      if(dropoff_assessment){
        position = *dropoff_assessment;
        faking_dropoff[position.x][position.y] = true;
      }else{
          abandoned_dropoffs.push_back(position);
          continue;
      }
    }
    game.me->dropoffs.push_back(make_shared<Dropoff>(game.me->id, -1, position.x, position.y));
    game.me->all_dropoffs.push_back(make_shared<Entity>(game.me->id, -1, position.x, position.y));
    game.game_map->at(position)->structure = game.me->dropoffs.back();
  }

  for(auto& position : abandoned_dropoffs){
    faking_dropoffs.erase(std::remove(faking_dropoffs.begin(), faking_dropoffs.end(), position), faking_dropoffs.end());
  }

  vector<shared_ptr<Ship>> ships;

  for (const auto &ship : me->ships) {
    // ship returned to shipyard (which is used as dropoff point)
    // currently we do not have any other dropoffs except shipyard
    if (game_map->has_my_structure(ship->position)) {
      // GOING_HOME[ship->id] = false;
      AVERAGE_TIME_TO_HOME =
          AVERAGE_TIME_TO_HOME * genes->average_time_home_decay +
          NUM_OF_MOVES_FROM_HOME[ship->id] *
              (1 - genes->average_time_home_decay);
      NUM_OF_MOVES_FROM_HOME[ship->id] = 0;  // at home (shipyard)
    }
    if (game_map->has_my_structure(ship->position) && faking_dropoff[ship->position.x][ship->position.y]){
      if(me->halite >= constants::DROPOFF_COST){
        me->halite -= constants::DROPOFF_COST;
        savings -= constants::DROPOFF_COST;
        faking_dropoff[ship->position.x][ship->position.y] = false;
        faking_dropoffs.erase(std::remove(faking_dropoffs.begin(), faking_dropoffs.end(), ship->position), faking_dropoffs.end());
        constructions.push_back(ship->make_dropoff());
      }else{
        direction_queue.emplace_back(ship, Direction::STILL);
      }
    }else if (!game_map->can_move(ship)) {
      navigate(ship, Direction::STILL, direction_queue);
    }else if(game_map->get_safe_from_enemy_moves_around(ship).size() == 0) {
      navigate(ship, Direction::STILL, direction_queue);
    }else {
      game.game_map->at(ship->position)->mark_safe();
      ships.push_back(ship);
    }
  }

  while ((ships = oplock_doStepNoStill(ships, direction_queue)).size() != 0) {
  }

  for (const auto &ship : me->ships) {
    // ship is always going away from home
    // when going to target point to collect halite
    NUM_OF_MOVES_FROM_HOME[ship->id]++;
  }

  log::log("doStep duration: " +
           to_string(duration_cast<duration<double>>(
                         high_resolution_clock::now() - t1)
                         .count()) +
           " ships: " + to_string(me->ships.size()));
  auto spawn_ship = me->halite - savings >= constants::SHIP_COST &&
             !(game_map->at(me->shipyard->position)->is_occupied()) &&
             should_ship_new_ship();

  if(spawn_ship){
    constructions.push_back(command::spawn_ship());
  }
  return constructions;
}

int main(int argc, const char *argv[]) {

  _candidates.reserve(constants::WIDTH * constants::HEIGHT * 100);
  // At this point "game" variable is populated with initial map data.
  // This is a good place to do computationally expensive start-up
  // pre-processing. As soon as you call "ready" function below, the 2 second
  // per turn timer will start.
  game.ready(__FILE__);
  genes = make_shared<Genes>(argc, argv);
  srand(genes->seed);
  // log::log("Successfully created bot! My Player ID is " +
  // to_string(game.my_id) + ". Bot rng seed is " + to_string(genes->seed) +
  // ".");

  ;
  vector<tuple<shared_ptr<Ship>, Direction>> direction_queue;
  while (1) {
    game.update_frame();

    direction_queue.resize(0);
    direction_queue.reserve(game.me->ships.size());

    vector<Command> command_queue = doStep(direction_queue);

    for (auto &[ship, direction] : direction_queue) {
      command_queue.push_back(command::move(ship->id, direction));
    }

    if (!game.end_turn(command_queue)) {
      break;
    }
  }

  return 0;
}

#include "hlt/game.hpp"
#include "hlt/constants.hpp"
#include "hlt/log.hpp"
#include "hlt/genes.hpp"

#include <random>
#include <ctime>
#include <unordered_map>
#include <optional>

using namespace std;
using namespace hlt;

unique_ptr<Genes> genes;
Game game;
shared_ptr<Player> me;
//
//
// Direction greedySquareMove(const shared_ptr<Ship> ship, const Position& target){
//   auto directions = game.game_map->get_safe_moves(ship->position, target);
//   return directions[rand()%directions.size()];
// }
//
// pair<int, shared_ptr<Entity>> getMinDistanceToDropoff(const Position& position, const vector<shared_ptr<Entity>>& dropoffs){
//   pair<int, shared_ptr<Entity>>  minDistance;
//   minDistance.first = 999999999;
//   for(const auto& dropoff : dropoffs){
//     int cur_distance = game.game_map->calculate_distance(position, dropoff->position);
//     if(cur_distance < minDistance.first){
//       minDistance.first = cur_distance;
//       minDistance.second = dropoff;
//     }
//   }
//   return minDistance;
// }
//
// std::optional<Direction> isRecallTime(const shared_ptr<Ship> ship){
//   auto minDstDropoff = getMinDistanceToDropoff(ship->position, me->all_dropoffs);
//   if(constants::MAX_TURNS - game.turn_number - genes->extra_time_for_recall
//     > minDstDropoff.first){
//       return {};
//   }
//   return greedySquareMove(ship, minDstDropoff.second->position);
//
// }





bool GOING_HOME[1000] = {false};
int NUM_OF_MOVES_FROM_HOME[1000] = {0};
int AVERAGE_TIME_TO_HOME = 0;

// void doStep(vector<Command>& command_queue){
//   me = game.me;
//   unique_ptr<GameMap>& game_map = game.game_map;
//
//
//   for (const auto& ship : me->ships) {
//       {
//         auto cell = game_map->at(ship->position);
//         if(cell->has_structure() && cell->structure->owner == me->id){
//           GOING_HOME[ship->id] = false;
//           AVERAGE_TIME_TO_HOME = AVERAGE_TIME_TO_HOME * genes->average_time_home_decay + NUM_OF_MOVES_FROM_HOME[ship->id] * (1-genes->average_time_home_decay);
//           NUM_OF_MOVES_FROM_HOME[ship->id] = 0;
//         }
//       }
//
//       if(!game_map->can_move(ship)){
//         command_queue.push_back(ship->stay_still());
//       }
//       // else if(auto recall=isRecallTime(ship)){
//       //   command_queue.push_back(command::move(ship->id,recall.value()));
//       // }
//       else{
//           Direction random_direction = ALL_CARDINALS[rand() % 4];
//           command_queue.push_back(ship->move(random_direction));
//       }
//   }
//
//   if (
//       game.turn_number <= 200 &&
//       me->halite >= constants::SHIP_COST &&
//       !game_map->at(me->shipyard)->is_occupied())
//   {
//       command_queue.push_back(me->shipyard->spawn());
//   }
// }

int main(int argc, char* argv[]) {

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
      // doStep(command_queue);

      if (!game.end_turn(command_queue)) {
          break;
      }
    }






    return 0;
}

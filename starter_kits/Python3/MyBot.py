#!/usr/bin/env python3
# Python 3.6

# Import the Halite SDK, which will let you interact with the game.
import hlt

# This library contains constant values.
from hlt import constants

# This library contains direction metadata to better interface with the game.
from hlt.positionals import Direction, Position

from hlt.commands import CONSTRUCT

# Logging allows you to save messages for yourself. This is required because the regular STDOUT
#   (print statements) are reserved for the engine-bot communication.
import logging
import os
import argparse
import random

parser = argparse.ArgumentParser()

GENES = {}
arguments_list = []
def add_argument(gene_name, mn, mx, default_value):
    arguments_list.append((gene_name, mn, mx))
    length = mx - mn
    parser.add_argument("--"+gene_name, type=float, default=(default_value-mn)*1.0/length)

add_argument("EXTRA_TIME_FOR_RECALL", 0, 10, 5)
add_argument("TRAFFIC_CONTROLLER_DISTANCE_MARGIN", 0, 10, 5)
add_argument("SHIP_SHIPS_UNTIL_TURN", 100, 300, 200)
add_argument("GREEDY_WALK_RANDOMISATION_MARGIN", 0, 50, 1)

args = parser.parse_args()
for gene_name, mn, mx in arguments_list:
    length = mx - mn
    GENES[gene_name] = vars(args)[gene_name] * length + mn

""" <<<Game Begin>>> """

# This game object contains the initial game state.
game = hlt.Game()
# At this point "game" variable is populated with initial map data.
# This is a good place to do computationally expensive start-up pre-processing.
# As soon as you call "ready" function below, the 2 second per turn timer will start.
game.ready("MyPythonBot-32")

# create file handler which logs even debug messages
_fh = logging.FileHandler(f'guru99-{game.my_id}.log')
_fh.setLevel(logging.DEBUG)
formatter = logging.Formatter('%(levelname)s-%(funcName)s- %(message)s')
_fh.setFormatter(formatter)
logger = logging.getLogger('')
logger.setLevel(logging.INFO)
logger.addHandler(_fh)

# Now that your bot is initialized, save a message to yourself in the log file with some important information.
#   Here, you log here your id, which you can always fetch from the game object by using my_id.
#logger.warn("Successfully created bot! My Player ID is {}.".format(game.my_id))

""" <<<Game Loop>>> """

# EXPERIMENT_ID = 0
# def create_experiment(message):
#     EXPERIMENT_ID = EXPERIMENT_ID - 1
#
#     if constants.DEBUG:
#         #logger.warn(f'creating experiemnt "{message}" with id = {EXPERIMENT_ID}')
#
#     return EXPERIMENT_ID

_DROPOFFS = None
def get_dropoffs():
    return _DROPOFFS

def push_decision(ship, directions):
    global command_queue
    global game_map

    if directions == CONSTRUCT:
        #logger.info(f'CONSTRUCT - {ship}; turn={game.turn_number}')
        command_queue.append(ship.make_dropoff())
    else:
        directions = list(filter(None, directions))
        direction, next_position = game_map.navigate(ship, directions)
        #logger.info(f'NORMAL - {ship}; direction = {direction}; next_position={next_position}; turn={game.turn_number}')
        command_queue.append(ship.move(direction))

def can_move(ship):
    global game_map

    if ship.halite_amount >= game_map[ship.position].halite_amount // constants.MOVE_COST_RATIO:
        #logger.debug(f'{ship} can move; turn={game.turn_number}')
        return True
    else:
        #logger.debug(f'{ship} can not move; turn={game.turn_number}')
        return False

def chose_random_move(ship, directions):
    push_decision(ship, directions)

def greedy_chose_square_move(ship, moves):
    global game_map

    if len(moves) == 0:
        logger.debug(f'{ship} had no moves, stay; turn={game.turn_number}')
        moves.append(Direction.Still)

    if len(moves) == 1:
        chosen_direction = moves[0]
    else:
        sorted_moves = sorted(moves, key=lambda move: game_map[ship.position.directional_offset(move)].halite_amount, reverse=True)
        if abs(game_map[ship.position.directional_offset(sorted_moves[0])].halite_amount//constants.MOVE_COST_RATIO - game_map[ship.position.directional_offset(sorted_moves[0])].halite_amount//constants.MOVE_COST_RATIO) < GENES["GREEDY_WALK_RANDOMISATION_MARGIN"]:
            chosen_direction = random.choice(sorted_moves)
        else:
            chosen_direction = sorted_moves[0]
    logger.debug(f'{ship} chose move {chosen_direction}; turn={game.turn_number}')
    push_decision(ship, [chosen_direction])

def is_time_to_recall(ship):
    global game_map
    is_it = constants.MAX_TURNS - game.turn_number - GENES["EXTRA_TIME_FOR_RECALL"] <= min([game_map.calculate_distance(ship.position,  target) for target in get_dropoffs()])
    if is_it:
        #logger.info(f'{ship}  is_it={is_it}; turn={game.turn_number}')
        pass
    return is_it

_ONCE_INIT = (None, None, -1)
dp = [[[_ONCE_INIT for _y in range(constants.HEIGHT)] for _x in range(constants.WIDTH)] for _turn in range(2+int((constants.HEIGHT+constants.WIDTH)*1.5))]
MARK = 1

def get_dp(turn, position):
    if dp[turn][position.x][position.y][2] != MARK:
        return None
    else:
        return dp[turn][position.x][position.y]

def set_dp(turn, position, value):
    dp[turn][position.x][position.y] = value

def compute_dp(ship, targets, recall=False):
    global game_map
    global MARK

    target = sorted(targets, key=lambda x: game_map.calculate_distance(ship.position, x))[0]

    MARK += 1

    distance_to_target = game_map.calculate_distance(ship.position, target)
    len_dp = 2 + int(distance_to_target*1.5)
    moves_closer = tuple(game_map.get_unsafe_moves(ship.position, target))

    if not recall:
        if distance_to_target < GENES["TRAFFIC_CONTROLLER_DISTANCE_MARGIN"] and len(moves_closer) == 1:
            if target not in TRAFFIC_CONTROLLER:
                TRAFFIC_CONTROLLER[target] = {}

            if len(TRAFFIC_CONTROLLER[target]) == 4:
                if TRAFFIC_CONTROLLER[target][moves_closer] == False:
                    return []
            elif len(TRAFFIC_CONTROLLER[target]) == 3 and moves_closer not in TRAFFIC_CONTROLLER[target]:
                TRAFFIC_CONTROLLER[target][moves_closer] = False
                return []
            else:
                TRAFFIC_CONTROLLER[target][moves_closer] = True

    y_move = Direction.North
    x_move = Direction.East
    if Direction.South in moves_closer:
        y_move = Direction.South
    if Direction.West in moves_closer:
        x_move = Direction.West

    set_dp(0, ship.position, (ship.halite_amount, None, MARK))

    for cur_turn in range(len_dp):
        cur_edge_position = ship.position
        while True: # X
            cur_position = cur_edge_position
            was_not_none = False
            while True: # y
                cur_dp_state = get_dp(cur_turn, cur_position)
                if cur_dp_state is None:
                    if was_not_none:
                        break
                else:
                    was_not_none = True
                    if cur_position == ship.position:
                        moves = game_map.get_safe_moves(source=ship.position, target=target, recall=recall)
                    else:
                        moves = game_map.get_unsafe_moves(cur_position, target)

                    for move in moves:
                        next_pos = cur_position.directional_offset(move)
                        cur_halite = cur_dp_state[0]
                        halite_to_grab = game_map[cur_position].halite_amount

                        def dp_update():
                            halite_left = cur_halite - halite_to_grab//constants.MOVE_COST_RATIO
                            if halite_left > 0:
                                next_turn = cur_turn + stay_turns + 1
                                if next_turn < len_dp:
                                    target_dp = get_dp(next_turn, next_pos)
                                    if target_dp is None or target_dp[0] < halite_left:
                                        move_to_set = cur_dp_state[1]
                                        if move_to_set is None:
                                            if stay_turns > 0:
                                                move_to_set = Direction.Still
                                            else:
                                                move_to_set = move
                                        set_dp(next_turn, next_pos, (halite_left, move_to_set, MARK))

                        stay_turns = 0
                        while True:
                            dp_update()

                            if cur_halite == constants.MAX_HALITE or halite_to_grab//constants.EXTRACT_RATIO == 0:
                                break

                            stay_turns += 1
                            cur_halite += halite_to_grab//constants.EXTRACT_RATIO
                            cur_halite = min(cur_halite, constants.MAX_HALITE)
                            halite_to_grab -= halite_to_grab//constants.EXTRACT_RATIO

                if cur_position.y == target.y:
                    break
                cur_position = cur_position.directional_offset(y_move)

            if cur_edge_position.x == target.x:
                break
            cur_edge_position = cur_edge_position.directional_offset(x_move)

    return [(num_turn, dp[num_turn][target.x][target.y][1], dp[num_turn][target.x][target.y][0]) for num_turn in range(len_dp) if dp[num_turn][target.x][target.y][2] == MARK]


def go_to_point_fast(ship, targets, recall=False):
    global game_map

    num_turn_and_dir = compute_dp(ship, targets, recall)
    #logger.debug(f'dp_results= {num_turn_and_dir}; turn={game.turn_number}')
    if len(num_turn_and_dir) == 0:
        #logger.info(f'{ship}  targets={targets}; recall={recall} stay still, meh; turn={game.turn_number}')
        push_decision(ship, [Direction.Still])
    else:
        #logger.info(f'{ship}  targets={targets}; recall={recall} go_to={num_turn_and_dir[0][1]}; turn={game.turn_number}')
        push_decision(ship, [num_turn_and_dir[0][1]])

def go_home_fast(ship, recall=False):
    targets = get_dropoffs()
    #logger.info(f'{ship} recall={recall} targets={targets}; turn={game.turn_number}')
    go_to_point_fast(ship, targets, recall)

def go_to_point_efficient(ship, targets, recall=False):
    global game_map

    num_turn_and_dir = compute_dp(ship, targets, recall)
    #logger.debug(f'dp_results= {num_turn_and_dir}; turn={game.turn_number}')

    if len(num_turn_and_dir) == 0:
        #logger.info(f'{ship} recall={recall} targets={targets}; turn={game.turn_number} stay_still_point')
        push_decision(ship, [Direction.Still])
    else:
        best = None
        for num_turn, direction, halite in num_turn_and_dir:
            cur = (halite*1.0/(get_state(ship, NUM_OF_MOVES_FROM_HOME) + num_turn), direction)
            if best is None or cur > best:
                best = cur
        #logger.info(f'{ship} recall={recall} targets={targets}; best={best}; turn={game.turn_number}')
        push_decision(ship, [best[1]])

def go_home_efficient(ship, recall=False):
    targets = get_dropoffs()
    #logger.info(f'{ship} recall={recall} targets={targets}; turn={game.turn_number}')
    go_to_point_efficient(ship, targets, recall)

GO_HOME_RECALL = 0
GO_HOME_EFFICIENT = 1
NUM_OF_MOVES_FROM_HOME = 2
DROPOFF_SHIP = 3
ship_STATE = []

def init_state():
    return [False, False, 0, None]

def get_state(ship, quest):
    global ship_STATE

    if ship_STATE[ship.id][quest] is not None:
        #logger.info(f'{ship} quest={quest} result={ship_STATE[ship.id][quest]}; turn={game.turn_number}')
        pass
    else:
        #logger.debug(f'{ship} quest={quest} result={ship_STATE[ship.id][quest]}; turn={game.turn_number}')
        pass
    return ship_STATE[ship.id][quest]

def set_state(ship, quest, value):
    global ship_STATE

    #logger.info(f'{ship} quest={quest} value={value}; turn={game.turn_number}')
    ship_STATE[ship.id][quest] = value

points_in_use = set()
ship_pair_used = [[-1 for _y in range(constants.HEIGHT)] for _x in range(constants.WIDTH)]

def pair_ships(ships):
    global game_map
    global MARK

    MARK += 1
    ship_candidates = []
    ship_satisfied = [False] * len(ships)
    for x in range(constants.WIDTH):
        for y in range(constants.HEIGHT):
            target = Position(x, y, False)
            target_halite_amount = game_map[target].halite_amount
            if target_halite_amount//constants.EXTRACT_RATIO == 0:
                continue

            for i in range(len(ships)):
                ship = ships[i]

                distance = 1.0 * game_map.calculate_distance(ship.position, target) #+ min([game_map.calculate_distance(target, dropoff.position) for dropoff in [me.shipyard] + me.get_dropoffs()])
                ship_candidates.append((i, target, min(constants.MAX_HALITE - ship.halite_amount, target_halite_amount) / (distance+1)))


    for i, target, efficiency in sorted(ship_candidates, key=lambda arg: arg[2], reverse=True):

        if ship_satisfied[i] == True:
            continue
        if ship_pair_used[target.x][target.y] == MARK:
            continue
        ship = ships[i]
        safe_moves = game_map.get_safe_moves(ship.position, target)
        if len(safe_moves) == 0 and target != ship.position:
            continue
        ship_pair_used[target.x][target.y] = MARK
        ship_satisfied[i] = True
        greedy_chose_square_move(ship, safe_moves)

    for i in range(len(ship_satisfied)):
        if ship_satisfied[i] == False:
            ship = ships[i]

            #logger.warn(f'Failed to satisfy {str(ships[i])}')
            push_decision(ship, [Direction.Still])

last_dropoff = 0
def is_ready_for_dropoff():
    global last_dropoff
    return False

    if last_dropoff//15 < len(me.get_ships())//15:
        #logger.info(f'ready last = {last_dropoff} now = {len(me.get_ships())}; turn={game.turn_number}')
        return True
    else:
        #logger.debug(f'not ready last = {last_dropoff} now = {len(me.get_ships())}; turn={game.turn_number}')
        return False

def get_dropoff_efficiency(centre):
    global game_map

    value = 0
    for x in range(constants.WIDTH):
        for y in range(constants.HEIGHT):
            position = Position(x, y, False)
            cell = game_map[position]
            distance = game_map.calculate_distance(centre, position) + 1.0

            if cell.is_empty:
                sub_value = cell.halite_amount
            elif cell.has_structure:
                sub_value = -1000
            elif cell.is_occupied:
                sub_value = -100

            value += sub_value / distance

    return value

def find_best_cluster():
    global game_map

    candidates = []
    for x in range(constants.WIDTH):
        for y in range(constants.HEIGHT):
            target = Position(x, y, False)
            if not game_map[target].is_empty:
                continue

            square_sum = 0
            for player in game.players.values():
                mn = 999999
                for item in player.get_dropoffs() + [player.shipyard]:
                    mn = min(mn,  game_map.calculate_distance(target, item.position))
                square_sum += mn * mn
            candidates.append((target, square_sum))

    candidates = [(target, get_dropoff_efficiency(target)) for target, _ in sorted(candidates, key=lambda x: x[1], reverse=True)[:30]]
    return sorted(candidates, key=lambda x: x[1], reverse=True)[0][0]

def send_for_dropoff(full_ships):
    global game_map
    if len(full_ships) == 0:
        return None, None

    target = find_best_cluster()
    return sorted(full_ships, key=lambda ship: game_map.calculate_distance(target, ship.position))[0], target


SAVINGS = 0
TRAFFIC_CONTROLLER = None
while True:
    TRAFFIC_CONTROLLER = {}
    #logger.error('\n\n\n\n\n\n\n\n')
    # This loop handles each turn of the game. The game object changes every turn, and you refresh that state by
    #   running update_frame().
    game.update_frame()
    # You extract player metadata and the updated map metadata here for convenience.
    me = game.me
    game_map = game.game_map

    _DROPOFFS = (me.shipyard.position,) + tuple([dropoff.position for dropoff in me.get_dropoffs()])
    game_map.init(me.shipyard.owner)

    # A command queue holds all the commands you will run this turn. You build this list up and submit it at the
    #   end of the turn.
    command_queue = []
    looking_for_point = []
    ready_to_go_home_ships = []

    for ship in sorted(me.get_ships(), key=lambda x: x.halite_amount, reverse=True):
        #logger.info(f'get_ship: turn={game.turn_number} {ship}')
        while ship.id >= len(ship_STATE):
            ship_STATE.append(init_state())

        # For each of your ships, move randomly if the ship is on a low halite location or the ship is full.
        #   Else, collect halite.
        #logger.info(f'is_drop_place {ship} structure_type={game_map[ship.position].structure_type}')
        if game_map.is_drop_place(game_map[ship.position]):
            #set_state(ship, GO_HOME_RECALL, False) let it stay there
            set_state(ship, GO_HOME_EFFICIENT, False)
            set_state(ship, NUM_OF_MOVES_FROM_HOME, 0)
            #logger.info(f'dropped_bag: {ship} turn={game.turn_number}')

        if ship.position == get_state(ship, DROPOFF_SHIP):
            if me.halite_amount >= constants.DROPOFF_COST:
                SAVINGS -= constants.DROPOFF_COST
                me.halite_amount -= constants.DROPOFF_COST
                set_state(ship, DROPOFF_SHIP, None)
                push_decision(ship, CONSTRUCT)
                #logger.info(f'dropoff_ship_constructed: {ship} turn={game.turn_number}')
            else:
                push_decision(ship, [Direction.Still])
                #logger.info(f'dropoff_ship_on_spot_poor: {ship} turn={game.turn_number}')

        else:
            if get_state(ship, DROPOFF_SHIP) is not None:
                target = get_state(ship, DROPOFF_SHIP)
                #logger.info(f'dropoff_ship_going {ship} target={target} turn={game.turn_number}')
                if not game_map[target].is_empty:
                    set_state(ship, DROPOFF_SHIP, None)
                    SAVINGS -= constants.DROPOFF_COST
                    #logger.info(f'dropoff_ship_reverted: {ship} savings={SAVINGS} turn={game.turn_number}')


            if get_state(ship, DROPOFF_SHIP) is not None:
                target = get_state(ship, DROPOFF_SHIP)
                go_to_point_fast(ship, [target])
                #logger.info(f'dropoff_ship_going: {ship} target={target} turn={game.turn_number}')
            elif get_state(ship, GO_HOME_RECALL) or is_time_to_recall(ship):
                set_state(ship, GO_HOME_RECALL, True)
                go_home_fast(ship, True)
                #logger.info(f'go_home_fast: {ship} turn={game.turn_number}')
            elif get_state(ship, GO_HOME_EFFICIENT) or ship.halite_amount >= constants.MAX_HALITE * 90/100:
                ready_to_go_home_ships.append(ship)
                #logger.info(f'go_home_efficient: {ship} turn={game.turn_number}')
            elif not can_move(ship):
                push_decision(ship, [Direction.Still])
                #logger.info(f'can_not_move: {ship} turn={game.turn_number}')
            else:
                looking_for_point.append(ship)
                #logger.info(f'looking_for_target: {ship} turn={game.turn_number}')

        set_state(ship, NUM_OF_MOVES_FROM_HOME, get_state(ship, NUM_OF_MOVES_FROM_HOME) + 1)

    pair_ships(looking_for_point)

    if is_ready_for_dropoff():
        SAVINGS += constants.DROPOFF_COST
        dropoff_ship, dropoff_position = send_for_dropoff(ready_to_go_home_ships)
        if dropoff_position is not None:
            last_dropoff = len(me.get_ships())
            set_state(dropoff_ship, DROPOFF_SHIP, dropoff_position)
            go_to_point_fast(dropoff_ship, [dropoff_position])

    for ship in ready_to_go_home_ships:
        if get_state(ship, DROPOFF_SHIP) is None:
            set_state(ship, GO_HOME_EFFICIENT, True)
            go_home_efficient(ship)

    # If the game is in the first 200 turns and you have enough halite, spawn a ship.
    # Don't spawn a ship if you currently have a ship at port, though - the ships will collide.

    if me.halite_amount - SAVINGS >= constants.SHIP_COST and not game_map[me.shipyard].is_occupied and game.turn_number <= GENES["SHIP_SHIPS_UNTIL_TURN"]:
        command_queue.append(me.shipyard.spawn())

    # Send your moves back to the game environment, ending this turn.
    game.end_turn(command_queue)

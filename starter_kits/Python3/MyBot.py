#!/usr/bin/env python3
# Python 3.6

# Import the Halite SDK, which will let you interact with the game.
import hlt

# This library contains constant values.
from hlt import constants

# This library contains direction metadata to better interface with the game.
from hlt.positionals import Direction, Position

# This library allows you to generate random numbers.
import random

# Logging allows you to save messages for yourself. This is required because the regular STDOUT
#   (print statements) are reserved for the engine-bot communication.
import logging

""" <<<Game Begin>>> """

# This game object contains the initial game state.
game = hlt.Game()
# At this point "game" variable is populated with initial map data.
# This is a good place to do computationally expensive start-up pre-processing.
# As soon as you call "ready" function below, the 2 second per turn timer will start.
game.ready("MyPythonBot")

# Now that your bot is initialized, save a message to yourself in the log file with some important information.
#   Here, you log here your id, which you can always fetch from the game object by using my_id.
logging.info("Successfully created bot! My Player ID is {}.".format(game.my_id))

""" <<<Game Loop>>> """

EXPERIMENT_ID = 0
def create_experiment(message):
    EXPERIMENT_ID = EXPERIMENT_ID - 1

    if constants.DEBUG:
        logging.INFO(f'creating experiemnt "{message}" with id = {EXPERIMENT_ID}')

    return EXPERIMENT_ID

def push_decision(ship, directions):
    global command_queue
    global game_map

    directions = list(filter(None, directions))
    direction, next_position = game_map.navigate(ship, directions)
    command_queue.append(ship.move(direction))

def can_move(ship):
    global game_map

    if ship.halite_amount >= game_map[ship.position].halite_amount * 10 // 100:
        return True
    else:
        return False

def chose_random_move(ship, directions):
    global command_queu

    push_decision(ship, directions)

def greedy_chose_square_move(ship, moves):
    global command_queue
    global game_map

    if len(moves) == 0:
        moves.append(Direction.Still)

    sorted_moves = sorted(moves, key=lambda move: game_map[ship.position.directional_offset(move)].halite_amount)
    chosen_direction = sorted_moves[0]
    push_decision(ship, [chosen_direction])

def is_time_to_recall(ship):
    global game_map

    return constants.MAX_TURNS - game.turn_number - 5 <= game_map.calculate_distance(ship.position,  me.shipyard.position)

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

def compute_dp(ship, target, recall=False):
    global game_map
    global MARK

    MARK += 1

    len_dp = 2 + int(game_map.calculate_distance(ship.position, target)*1.5)

    moves_closer = game_map.get_unsafe_moves(ship.position, target)

    y_move = Direction.North
    x_move = Direction.East
    if Direction.South in moves_closer:
        y_move = Direction.South
    if Direction.West in moves_closer:
        x_move = Direction.West

    # if game_map.calculate_distance(ship.position, target) > 5:
    #     f.write('GIVI\n' + str(ship) + '\n' + str(target) + '\n'+ str(dp) + '\n')
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
                    # if was_not_none==True:
                    #     f.write(f'GIO {str(was_not_none)} {str(ship)} {str(target)} {str(cur_position)} {str(cur_dp_state)}\n')
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

                            if cur_halite == constants.MAX_HALITE or halite_to_grab//constants.MOVE_COST_RATIO < 10:
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

    # if game_map.calculate_distance(ship.position, target) > 5:
    #     f.write('GIVI\n' + str(ship) + '\n' + str(target) + '\n'+ str(dp) + '\n')
    #     return 1/0
    return [(num_turn, dp[num_turn][target.x][target.y][1], dp[num_turn][target.x][target.y][0]) for num_turn in range(len_dp) if dp[num_turn][target.x][target.y][2] == MARK]


def go_to_point_fast(ship, target, recall=False):
    global game_map

    num_turn_and_dir = compute_dp(ship, target, recall)
    #f.write(f'GIVI {str(ship)} {str(target)} {str(num_turn_and_dir)} \n')
    if len(num_turn_and_dir) == 0:
        push_decision(ship, [Direction.Still])
    else:
        push_decision(ship, [num_turn_and_dir[0][1]])

def go_home_fast(ship, recall=False):
    go_to_point_fast(ship, me.shipyard.position, recall)
# f = open("guru99.txt","w+")
def go_to_point_efficient(ship, target, recall=False):
    global game_map

    num_turn_and_dir = compute_dp(ship, target, recall)

    # f.write(str(ship) + " " +  str(num_turn_and_dir) + '\n')
    if len(num_turn_and_dir) == 0:
        push_decision(ship, [Direction.Still])
    else:
        best = None
        for num_turn, direction, halite in num_turn_and_dir:
            cur = (halite*1.0/(get_state(ship, NUM_OF_MOVES_FROM_HOME) + num_turn), direction)
            if best is None or cur > best:
                best = cur
        # f.write(str(ship) + " " + str(best[1]) + " " + str(num_turn_and_dir))
        push_decision(ship, [best[1]])

def go_home_efficient(ship, recall=False):
    go_to_point_efficient(ship, me.shipyard.position, recall)

def exclude_going_closer(position, safe_moves, dst):
    global game_map
    global game

    current_distance = game_map.calculate_distance(position, dst)
    if current_distance < 3 + game.turn_number/10:
        return safe_moves
    return list(set(safe_moves) - set(game_map.get_unsafe_moves(position, dst)))

GO_HOME_RECALL = 0
GO_HOME_EFFICIENT = 1
NUM_OF_MOVES_FROM_HOME = 2
# GO_TO_POIINT = 3
ship_STATE = []

def init_state():
    return [False, False, 0, None]

def get_state(ship, quest):
    return ship_STATE[ship.id][quest]

def set_state(ship, quest, value):
    ship_STATE[ship.id][quest] = value

points_in_use = set()
ship_pair_used = [[-1 for _y in range(constants.HEIGHT)] for _x in range(constants.WIDTH)]

def pair_ships(ships):
    global game_map
    global MARK

    MARK += 1
    ship_candidates = []
    ship_satisfied = [False for _ in range(len(ships))]
    for x in range(constants.WIDTH):
        for y in range(constants.HEIGHT):
            target = Position(x, y, False)
            target_halite_amount = game_map[target].halite_amount
            if target_halite_amount//constants.EXTRACT_RATIO < 10:
                continue

            for i in range(len(ships)):
                ship = ships[i]

                distance = game_map.calculate_distance(ship.position, target) * 1.0
                ship_candidates.append((i, target, min(constants.MAX_HALITE - ship.halite_amount, target_halite_amount) / (distance+1)))


    for i, target, efficiency in sorted(ship_candidates, key=lambda arg: arg[2], reverse=True):

        if ship_satisfied[i] == True:
            continue
        if ship_pair_used[target.x][target.y] == MARK:
            continue
        # f.write(f'{str(i)} {str(target)} {str(efficiency)}\n')
        ship = ships[i]
        safe_moves = game_map.get_safe_moves(ship.position, target)
        if len(safe_moves) == 0 and target != ship.position:
            continue
        ship_pair_used[target.x][target.y] = MARK
        ship_satisfied[i] = True
        push_decision(ship, safe_moves)
    #f.write('\n\n')

    for i in range(len(ship_satisfied)):
        if ship_satisfied[i] == False:
            ship = ships[i]

            logging.info(f'Failed to satisfy {str(ships[i])}')
            push_decision(ship, [Direction.Still])


while True:
    # This loop handles each turn of the game. The game object changes every turn, and you refresh that state by
    #   running update_frame().
    game.update_frame()
    # You extract player metadata and the updated map metadata here for convenience.
    me = game.me
    game_map = game.game_map

    # A command queue holds all the commands you will run this turn. You build this list up and submit it at the
    #   end of the turn.
    command_queue = []
    looking_for_point = []
    for ship in sorted(me.get_ships(), key=lambda x: x.halite_amount, reverse=True):
        while ship.id >= len(ship_STATE):
            ship_STATE.append(init_state())

        # For each of your ships, move randomly if the ship is on a low halite location or the ship is full.
        #   Else, collect halite.
        if game_map.is_drop_place(game_map[ship.position]):
            #set_state(ship, GO_HOME_RECALL, False) let it stay there
            set_state(ship, GO_HOME_EFFICIENT, False)
            set_state(ship, NUM_OF_MOVES_FROM_HOME, 0)

        if get_state(ship, GO_HOME_RECALL) or is_time_to_recall(ship):
            set_state(ship, GO_HOME_RECALL, True)
            go_home_fast(ship, True)
        elif get_state(ship, GO_HOME_EFFICIENT) or ship.halite_amount >= constants.MAX_HALITE * 90/100:
            set_state(ship, GO_HOME_EFFICIENT, True)
            go_home_efficient(ship)
        elif not can_move(ship):
            push_decision(ship, [Direction.Still])
        else:
            looking_for_point.append(ship)

        set_state(ship, NUM_OF_MOVES_FROM_HOME, get_state(ship, NUM_OF_MOVES_FROM_HOME) + 1)

    pair_ships(looking_for_point)

    # If the game is in the first 200 turns and you have enough halite, spawn a ship.
    # Don't spawn a ship if you currently have a ship at port, though - the ships will collide.

    if me.halite_amount >= constants.SHIP_COST and not game_map[me.shipyard].is_occupied and game.turn_number <= 200:
        command_queue.append(me.shipyard.spawn())

    # Send your moves back to the game environment, ending this turn.
    game.end_turn(command_queue)

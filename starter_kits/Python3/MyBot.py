#!/usr/bin/env python3
# Python 3.6

# Import the Halite SDK, which will let you interact with the game.
import hlt

# This library contains constant values.
from hlt import constants

# This library contains direction metadata to better interface with the game.
from hlt.positionals import Direction

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

RATIO_TO_GO_HOME = 80

def push_decision(ship, direction):
    global next_map
    global command_queue

    position = ship.position.directional_offset(direction)
    next_map[position] = True
    command_queue.append(ship.move(direction))

def can_move(ship):
    global game_map

    if ship.halite_amount >= game_map[ship.position].halite_amount * 10 / 100:
        return True
    else:
        return False

def get_safe_moves(ship, moves, recall=False):
    global next_map

    if not can_move(ship):
        return []

    safe_moves = []
    for direction in moves:
        next_position = ship.position.directional_offset(direction)
        if (recall and next_position == me.shipyard.position) or next_position not in next_map:
            safe_moves.append(direction)
    return safe_moves

def chose_random_move(ship, moves):
    global command_queue

    if len(moves) == 0:
        moves.append(Direction.Still)
    chosen_direction = random.choice(moves)
    push_decision(ship, chosen_direction)

def greedy_chose_smallest_square_move(ship, moves):
    global command_queue
    global game_map

    if len(moves) == 0:
        moves.append(Direction.Still)

    chosen_direction = sorted(moves, key=lambda move: game_map[ship.position.directional_offset(move)].halite_amount, reverse=True)[0]
    push_decision(ship, chosen_direction)


def is_time_to_recall(ship):
    global game_map

    TURN_LIMIT = {32:401, 40:426, 48:451, 56:476, 64:501}
    return TURN_LIMIT[game_map.width] - game.turn_number - 5 <= game_map.calculate_distance(ship.position,  me.shipyard.position)


def go_to_point_fast(ship, position, recall):
    global game_map

    unsafe_moves = game_map.get_unsafe_moves(ship.position, position)
    safe_moves = get_safe_moves(ship, unsafe_moves, recall)
    greedy_chose_smallest_square_move(ship, safe_moves)

def go_home(ship, recall=False):
    go_to_point_fast(ship, me.shipyard.position, recall)

def go_home_full(ship):
    global game_map

    if ship.halite_amount < 900 and game_map[ship.position].halite_amount > 30:
        push_decision(ship, Direction.Still)
    else:
        go_home(ship)

def exclude_going_closer(position, safe_moves, dst):
    global game_map
    global game

    current_distance = game_map.calculate_distance(position, dst)
    if current_distance < 3 + game.turn_number/10:
        return safe_moves
    return [move for move in safe_moves if game_map.calculate_distance(position.directional_offset(move), dst) >= current_distance]


# def time_needed_to_go(src, dst, halite_amount):



STATE_is_going_home = {}

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
    next_map = {}
    for _, player in game.players.items():
        for ship in player.get_ships():
            next_map[ship.position] = True

    for ship in sorted(me.get_ships(), key=lambda x: x.halite_amount, reverse=True):
        # For each of your ships, move randomly if the ship is on a low halite location or the ship is full.
        #   Else, collect halite.
        if ship.id in STATE_is_going_home and ship.position == me.shipyard.position:
            del STATE_is_going_home[ship.id]

        if is_time_to_recall(ship):
            go_home(ship, True)
        elif ship.id in STATE_is_going_home or ship.halite_amount >= constants.MAX_HALITE * 90/100:
            STATE_is_going_home[ship.id] = True
            go_home_full(ship)
        elif can_move(ship) and (ship.halite_amount >= game_map[ship.position].halite_amount < constants.MAX_HALITE / 10 or ship.is_full):
            safe_moves = get_safe_moves(ship, Direction.get_all_cardinals())
            run_far_moves = exclude_going_closer(ship.position, safe_moves, me.shipyard.position)
            chose_random_move(ship, run_far_moves)
        else:
            push_decision(ship, Direction.Still)

    # If the game is in the first 200 turns and you have enough halite, spawn a ship.
    # Don't spawn a ship if you currently have a ship at port, though - the ships will collide.

    if game.turn_number <= 200 and me.halite_amount >= constants.SHIP_COST and not game_map[me.shipyard].is_occupied and me.shipyard.position not in next_map:
        command_queue.append(me.shipyard.spawn())

    # Send your moves back to the game environment, ending this turn.
    game.end_turn(command_queue)

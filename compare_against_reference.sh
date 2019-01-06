#!/usr/bin/env bash

g++-7 -std=c++17 -march=native -oFast -flto MyBot.cpp hlt/*.cpp -o MyBot
g++-7 -std=c++17 -march=native -oFast -flto MyBot.cpp hltref/*.cpp -o ReferenceBot
./halite --replay-directory replays/ -vvv --width 32 --height 32 "./MyBot" "./ReferenceBot"
./halite --replay-directory replays/ -vvv --width 48 --height 48 "./MyBot" "./ReferenceBot"
./halite --replay-directory replays/ -vvv --width 56 --height 56 "./MyBot" "./ReferenceBot"
./halite --replay-directory replays/ -vvv --width 64 --height 64 "./MyBot" "./ReferenceBot"

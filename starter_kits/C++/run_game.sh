#!/usr/bin/env bash

set -e
/usr/local/bin/g++-8 -std=c++17  -fprofile-generate *.cpp hlt/*.cpp -o MyBotProfile
g++ -std=c++17 -march=native -oFast -flto *.cpp hlt/*.cpp -o MyBot

./halite --replay-directory replays/ -vvv --width 32 --height 32 "./MyBotProfile" "./MyBot"

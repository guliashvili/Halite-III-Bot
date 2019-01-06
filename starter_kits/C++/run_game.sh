#!/usr/bin/env bash

g++ -std=c++17 -march=native -oFast -flto  -fprofile-generate=prof *.cpp hlt/*.cpp -o MyBotProfile
g++ -std=c++17 -march=native -oFast -flto *.cpp hlt/*.cpp -o MyBotTmp

./halite --replay-directory replays/ -vvv --width 64 --height 64 "./MyBotProfile" "./MyBotTmp"

g++ -std=c++17 -march=native -oFast -flto  --fprofile-use=prof *.cpp hlt/*.cpp -o MyBotProfile

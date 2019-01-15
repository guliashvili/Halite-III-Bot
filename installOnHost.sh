#!/usr/bin/env bash

g++-7 -std=c++17 -march=native -oFast -fprofile-generate=prof MyBot.cpp hlt/*.cpp -o MyBotProfile > /dev/null 2>&1
g++-7 -std=c++17 -march=native -oFast MyBot.cpp hlt/*.cpp -o MyBotTmp > /dev/null 2>&1

./haliteLinux --no-timeout --no-replay --no-logs --width 64 --height 64 "./MyBotProfile" "./MyBotTmp" > /dev/null 2>&1

g++-7 -std=c++17 -march=native -oFast -fprofile-use=prof MyBot.cpp hlt/*.cpp -o MyBot

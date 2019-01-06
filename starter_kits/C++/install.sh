#!/usr/bin/env bash

g++-7 -std=c++17 -march=native -oFast -flto -fprofile-generate=prof MyBot.cpps hlt/*.cpp -o MyBotProfile
g++-7 -std=c++17 -march=native -oFast -flto MyBot.cpps hlt/*.cpp -o MyBotTmp

./haliteLinux --no-timeout --no-replay --no-logs --width 64 --height 64 "./MyBotProfile" "./MyBotTmp"

g++-7 -std=c++17 -march=native -oFast -flto  -fprofile-use=prof MyBot.cpps hlt/*.cpp -o MyBot

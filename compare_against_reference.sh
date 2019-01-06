#!/usr/bin/env bash

CMD=halite
if [[ `uname -a | grep Linux` ]] ; then
	CMD=haliteLinux
fi

g++-7 -std=c++17 -march=native -oFast -flto MyBot.cpp hlt/*.cpp -o MyBot
g++-7 -std=c++17 -march=native -oFast -flto MyBot.cpp hltref/*.cpp -o ReferenceBot
./$CMD --replay-directory replays/ -vvv --width 32 --height 32 "./MyBot" "./ReferenceBot"
./$CMD --replay-directory replays/ -vvv --width 48 --height 48 "./MyBot" "./ReferenceBot"
./$CMD --replay-directory replays/ -vvv --width 56 --height 56 "./MyBot" "./ReferenceBot"
./$CMD --replay-directory replays/ -vvv --width 64 --height 64 "./MyBot" "./ReferenceBot"

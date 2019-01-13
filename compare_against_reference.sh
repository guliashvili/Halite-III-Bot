#!/usr/bin/env bash

CMD=halite
if [[ `uname -a | grep Linux` ]] ; then
	CMD=haliteLinux
fi

function playWith2 {
   stats=`./"$CMD" --replay-directory replays/ -vvv --width "$1" --height "$1" "./MyBot" "./ReferenceBot" --results-as-json  --no-compression | tee output-2-$1 | python readJson.py`
   echo "For 2 players on a $1x$1 grid we have the following stats: $stats" 
}

function playWith4 {
   stats=`./"$CMD" --replay-directory replays/ -vvv --width "$1" --height "$1" "./MyBot" "./ReferenceBot" "./MyBot" "./ReferenceBot" --results-as-json  --no-compression | tee output-4-$1 | python readJson.py`
   echo "For 4 players on a $1x$1 grid we have the following stats: $stats" 
}


g++ -std=c++17 -march=native -oFast MyBot.cpp hlt/*.cpp -o MyBot
g++ -std=c++17 -march=native -oFast ReferenceBot.cpp hltref/*.cpp -o ReferenceBot

playWith2 32
playWith2 48
playWith2 56
playWith2 64

printf "\n\n\n====4 player games====\n\n\n"

playWith4 32
playWith4 48
playWith4 56
playWith4 64




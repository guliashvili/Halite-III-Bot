#!/usr/bin/env bash

set -e
make clean
cmake .
 make
./halite --replay-directory replays/ -vvv --width 64 --height 64 "./MyBot" "./MyBotOld"

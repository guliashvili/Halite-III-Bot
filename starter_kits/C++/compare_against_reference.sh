#!/usr/bin/env bash

set -e
make clean
cmake .
make
./halite --replay-directory replays/ -vvv --width 32 --height 32 "./MyBot" "./ReferenceBot"
./halite --replay-directory replays/ -vvv --width 56 --height 56 "./MyBot" "./ReferenceBot"
./halite --replay-directory replays/ -vvv --width 64 --height 64 "./MyBot" "./ReferenceBot"

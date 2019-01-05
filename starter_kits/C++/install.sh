#!/usr/bin/env bash

set -e

mv CMakeLists.txt CMakeLists.txtbackup
cp CMakeListsProfile.txt CMakeLists.txt

cmake .
make
mv MyBot MyBotReference1

make clean
cmake -DCMAKE_CXX_FLAGS=-fprofile-generate -DCMAKE_C_COMPILER=/usr/local/bin/gcc-8 -DCMAKE_CXX_COMPILER=/usr/local/bin/g++-8 .
make
./halite --replay-directory replays/ -vvv --width 64 --height 64 "./MyBot" "./MyBotReference1"

mv CMakeLists.txtbackup CMakeLists.txt

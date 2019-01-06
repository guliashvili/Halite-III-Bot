#!/usr/bin/env bash

TARGET_PKG=/tmp/halite_pkg.zip
CUR_DIR=`pwd`
TMP_DIR=/tmp/halite_build

rm -rf $TMP_DIR ; mkdir -p $TMP_DIR
cp -r $CUR_DIR/* $TMP_DIR

rm -rf $TMP_DIR/replays
rm -rf $TMP_DIR/.git*

rm -f $TARGET_PKG
cd $TMP_DIR
zip -r /tmp/halite_pkg.zip .


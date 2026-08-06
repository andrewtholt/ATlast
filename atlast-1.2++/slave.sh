#!/usr/bin/env bash
#
set -x

git status .
echo $?

./build.sh clean && ./build.sh

./build.sh

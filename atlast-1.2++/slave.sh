#!/usr/bin/env bash
#
set -x

git status . | grep "nothing to commit, working tree clean"
echo $?

exit

./build.sh clean && ./build.sh

./build.sh

#!/usr/bin/env bash
#
#
set -x

git status . | grep "nothing to commit, working tree clean"
if [ $? -ne 0 ] ; then
    echo "Uncommited changes."
    exit 0
else
    git pull
fi

./build.sh clean && ./build.sh

./build.sh install


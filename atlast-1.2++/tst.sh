#!/usr/bin/bash
set -x

#if [ -z "$TEST" ]; then
#    echo "Empty"
#elif [ "$TEST" = "atlast" ]; then
#    echo "123 fred !"
case $MASTER in
    "atlast")
        echo "123 fred !";;
    *)
        echo "Empty";;
esac


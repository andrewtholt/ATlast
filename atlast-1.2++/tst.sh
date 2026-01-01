#!/usr/bin/bash
# set -x

VARNAME="fred"
VAR=$(ps -ef | tail -n +2 | wc -l)

case $MASTER in
    "atlast")
        printf "%s %s !" $VAR $VARNAME;;
    "python")
        printf "%s = %d" $VARNAME $VAR;;
    *)
        echo "SET MASTER";;
esac


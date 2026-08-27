#!/bin/bash

set -x

LIB=../../forthSrc/atlast/struct.atl
ENV=env.atl

cat $LIB $ENV ./msg.atl > ./tst.atl



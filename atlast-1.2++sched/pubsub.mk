CC=gcc
BASIC=""
ATLOBJ=linux.o
# ATLCONFIG=-DLINUX
ATLCONFIG=-DPUBSUB -DMQ -DLINUX
INCLUDE=-I/usr/local/lib/Small
LIBRARIES=-L/usr/local/lib -lsmall -lstdc++


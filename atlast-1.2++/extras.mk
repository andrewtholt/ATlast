CC=g++
CFLAGS=-std=c++11 -g -I/usr/local/include -DLIBSER -DATH
EXTRAS=""
ATLCONFIG=-DLINUX -DEXTRAS # -DSYSVIPC
ATLOBJ=linux.o extraFunc.o
# LIBRARIES=-lmosquitto
LIBRARIES = -L/usr/local/lib -lser


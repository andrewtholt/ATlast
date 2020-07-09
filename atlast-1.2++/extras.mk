CC=g++
CFLAGS=-std=c++11 -g -I/usr/local/include -DLIBSER -DATH
EXTRAS=""
# CPP_EXTRAS includes boost dependent words."
ATLCONFIG=-DLINUX -DEXTRAS # -DCPP_EXTRAS -DSYSVIPC
ATLOBJ=linux.o extraFunc.o
# LIBRARIES=-lmosquitto
LIBRARIES = -L/usr/local/lib -lser


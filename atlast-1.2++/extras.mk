CC=g++
# CFLAGS=-std=c++11 -g -I/usr/local/include -DLIBSER -DATH
CFLAGS=-std=c++11 -g -I/usr/local/include -DATH
EXTRAS=""
# CPP_EXTRAS includes boost dependent words."
ATLCONFIG=-DLINUX -DEXTRAS -DCPP_EXTRAS -DSYSVIPC
# ATLOBJ=linux.o extraFunc.o
ATLOBJ=linux.o
# LIBRARIES=-lmosquitto
LIBRARIES = -L/usr/local/lib -lser


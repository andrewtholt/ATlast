CC=g++
CFLAGS=-std=c++11 -g -I/usr/local/include -DATH
EXTRAS=""
ATLCONFIG=-DLINUX -DEXTRAS -DCPP_EXTRAS -DSYSVIPC
ATLOBJ=linux.o
LIBRARIES = -L/usr/local/lib -lser


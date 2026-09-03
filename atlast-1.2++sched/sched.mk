CC=g++
CFLAGS=-std=c++11 -g -I/usr/local/include -I/home/andrewtholt60/Source/VerySimpleDatabase
ATLCONFIG=-DLINUX -DALIGNMENT -DEXPORT -DREADONLYSTRINGS -DWORDSUSED -DSCHED -DATH -DSOCKET -DKV
ATLOBJ=linux.o sched.o
LIBRARIES=-lkv -L/home/andrewtholt60/Source/VerySimpleDatabase


CC=g++
CFLAGS=-std=c++11 -g -I/usr/local/include -I/home/andrewtholt60/Source/VerySimpleDatabase
ATLCONFIG=-DLINUX -DALIGNMENT -DEXPORT -DREADONLYSTRINGS -DWORDSUSED -DSCHED -DATH -DSOCKET -DKV -DSQLITE3 -DEXTRAS -DCPP_EXTRAS
ATLOBJ=linux.o sched.o extraFunc.o
LIBRARIES=-lkv -lsqlite3 -lhiredis # -L/home/andrewtholt60/Source/VerySimpleDatabase


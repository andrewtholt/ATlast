# CC=gcc
MQTT=""
ATLCONFIG=-DMQTT -DLINUX
ATLOBJ=mqtt.o linux.o ringBuffer.o
LIBRARIES=-L/usr/lib/x86_64-linux-gnu -lmosquitto


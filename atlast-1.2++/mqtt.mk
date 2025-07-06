# CC=gcc
MQTT=""
ATLCONFIG=-DFILEIO -DMQTT -DLINUX -DATH -DCPP_EXTRAS
ATLOBJ=mqtt.o linux.o ringBuffer.o
LIBRARIES=-L/usr/lib/x86_64-linux-gnu -lmosquitto


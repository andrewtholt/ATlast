
#	Unix makefile for ATLAST

VERSION = 1.2-Min
CC=g++ -std=c++11 -g -I/usr/local/include

# LIBRARIES = -lm
# LIBRARIES += -lrt -lpthread
# LIBRARIES = -L/usr/local/lib -lsmall -lstdc++ -lrt -lpthread
# LIBRARIES = -L/usr/local/lib -lser

ATLCONFIG += -DCUSTOM -DALIGNMENT -DEXPORT -DREADONLYSTRINGS 
LDFLAGS +=-Wl,--no-as-needed

# CFLAGS += $(ATLCONFIG) -Wfatal-errors -g -fPIC  -DLINUX -w -fpermissive # -Wwrite-strings
CFLAGS += $(ATLCONFIG) -Wl,--no-as-needed -Wfatal-errors -g -fPIC -w -fpermissive # -Wwrite-strings

ATLOBJ += atlast.o atlmain.o # extraFunc.o

APPS = atlast atlsmall # primdeftest

all:	$(APPS)

linux.o:	linux.c
	$(CC) $(CFLAGS) -c linux.c -o linux.o $(INCLUDE)

extraFunc.o:	extraFunc.cpp extraFunc.h
	g++  $(CFLAGS) -c extraFunc.cpp -o extraFunc.o

mqtt.o:	mqtt.c
	$(CC) $(CFLAGS) -c mqtt.c -o mqtt.o 

atlast: $(ATLOBJ) # mqtt.o
	$(CC) $(CFLAGS) $(INCLUDE) -Wl,--no-as-needed $(ATLOBJ) -o atlast $(LIBRARIES)

# CFLAGS += -I/usr/local/lib/Small

atlsmall: $(ATLOBJ) atlMinMain.c
	$(CC) $(CFLAGS) $(LDFLAGS) -Wl,--no-as-needed  atlMinMain.c atlast.o -o atlsmall $(LIBRARIES)

atlast.o:   atlast.c atldef.h atlcfig.h 
	$(CC) $(CFLAGS) $(INCLUDE) -c atlast.c -o atlast.o 

atldef.h:   atlast.h

atlmain.o:  atlmain.c atlast.h nvramrc.h
	$(CC) $(CFLAGS) $(INCLUDE)  -c atlmain.c -o atlmain.o 

primdeftest:
	$(CC) $(CFLAGS) primdeftest.c atlast.c -o primdeftest $(LIBRARIES)

#	Run the Atlast regression test

regression:
	echo testcons >/tmp/regin.tmp
	echo Test 1234 >>/tmp/regin.tmp
	echo ls atlast.c >>/tmp/regin.tmp
	echo  >>/tmp/regin.tmp
	echo  >>/tmp/regin.tmp
	./atlast -iregress </tmp/regin.tmp >/tmp/regout.tmp
	diff regout.txt /tmp/regout.tmp

clean:
	rm -f $(APPS)
	rm -f *.bak *.o *.dvi *.aux *.log
	rm -f core core.* cscope.out *.tar.gz
	rm -rf dist
#	( cd tex ; make clean )
	
dist:
	rm -rf dist
	mkdir dist
	mkdir dist/atlast-$(VERSION)
	cp -p atlast.c atlast.h tex/atlast.pdf atldef.h \
		atlast.html \
		atlmain.c COPYING log.txt Makefile MANIFEST \
		primdeftest.c \
		regout.txt regress.atl dist/atlast-$(VERSION)
	mkdir dist/atlast-$(VERSION)/tex
	cp -p tex/*.tex tex/*.sty tex/Makefile dist/atlast-$(VERSION)/tex
	find dist -type f -exec chmod 644 {} \;
	( cd dist ; tar cfvz ../atlast-$(VERSION).tar.gz atlast-$(VERSION) )
	rm -rf dist

lint:	lintatlast

lintatlast:
	lint atlast.c atlmain.c $(LIBRARIES) $(ATLCONFIG)

install:	atlast
	./install.sh
#	strip atlast
#	strip atlsmall
#	mv ./atlsmall /usr/local/bin
#	cp ./atlast /usr/local/bin
#	strip /usr/local/bin/atlast
#	strip atlsmall
#	mv ./atlsmall /usr/local/bin
#	mkdir -p /usr/local/lib/Small
#	cp tasks.h /usr/local/lib/Small



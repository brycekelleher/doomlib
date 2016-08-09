CXXFLAGS = -g -ggdb
LDFLAGS = -g -ggdb

all: lswad dumpwad dumpmap pictorgba

lswad: lswad.o doomlib.o
dumpwad: dumpwad.o doomlib.o
dumpmap: dumpmap.o doomlib.o
picinfo: picinfo.o
pictorgba: pictorgba.o

doomtri: doomtri.o doomlib.o

clean:
	rm -rf *.o
	rm -rf dumpwad dumpmap pictorgb

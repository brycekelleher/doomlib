CXXFLAGS = -g -O0 -ggdb
LDFLAGS = -g -O0 -ggdb

all: lswad dumpwad dumpmap dumptexture pictorgba

lswad: lswad.o doomlib.o
dumpwad: dumpwad.o doomlib.o
dumpmap: dumpmap.o doomlib.o
dumptexture: dumptexture.o doomlib.o
picinfo: picinfo.o
pictorgba: pictorgba.o

doomtri: doomtri.o doomlib.o

clean:
	rm -rf *.o
	rm -rf lswad dumpwad dumpmap picinfo pictorgb

all: lswad dumpwad dumpmap

lswad: lswad.o doomlib.o
dumpwad: dumpwad.o doomlib.o
dumpmap: dumpmap.o doomlib.o

doomtri: doomtri.o doomlib.o

clean:
	rm -rf *.o
	rm -rf dumpmap

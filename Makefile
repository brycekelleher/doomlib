all: dumpmap doomtri

dumpmap: dumpmap.o doomlib.o

doomtri: doomtri.o doomlib.o

clean:
	rm -rf *.o
	rm -rf dumpmap

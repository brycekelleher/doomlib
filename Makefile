all: dumpmap

dumpmap: dumpmap.o doomlib.o

clean:
	rm -rf *.o
	rm -rf dumpmap

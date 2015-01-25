CXXFLAGS	= -g -ggdb
OBJECTS = doomview.o

#ifeq ($(APPLE),1)
CXXFLAGS += -I/usr/X11R6/include -DGL_GLEXT_PROTOTYPES
LDFLAGS = -L/usr/X11R6/lib
LDLIBS	= -lGL -lglut
#endif

doomview: $(OBJECTS)

clean:
	rm -rf doomview $(OBJECTS)

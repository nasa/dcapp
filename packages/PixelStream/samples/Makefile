.PHONY: clean

OSSPEC := $(shell ../bin/PixelStream-config --osspec)
BINDIR := $(shell ../bin/PixelStream-config --bindir)

CXXFLAGS += -Wall
CXXFLAGS += $(shell ../bin/PixelStream-config --cflags)
CXXFLAGS += $(shell ../../TaraDraw/bin/TaraDraw-config --cflags)
CXXFLAGS += $(shell ../../utils/bin/utils-config --cflags)

LINK_LIBS += $(shell ../bin/PixelStream-config --libs)
LINK_LIBS += $(shell ../../TaraDraw/bin/TaraDraw-config --libs)
LINK_LIBS += $(shell ../../utils/bin/utils-config --libs)

ifeq ($(OSSPEC), MacOS)
    CXXFLAGS += -I/opt/X11/include
    LINK_LIBS += -framework OpenGL -framework AppKit
else
    CXXFLAGS += -I/usr/X11R6/include
    LINK_LIBS += -L/usr/X11R6/lib -lX11 -lXi -lXmu -lGL -lGLU
endif

$(BINDIR)/PixelStreamReader:
	mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) PixelStreamReader.cc -o $@ $(LINK_LIBS)

clean:
	rm -rf $(BINDIR)

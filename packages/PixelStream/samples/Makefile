.PHONY: clean

OSSPEC := $(shell ../bin/PixelStream-config --osspec)
BINDIR := $(shell ../bin/PixelStream-config --bindir)

CXXFLAGS += -Wall -Wextra 
CXXFLAGS += $(shell ../bin/PixelStream-config --cflags)
CXXFLAGS += $(shell ../../TaraDraw/bin/TaraDraw-config --cflags)
CXXFLAGS += $(shell ../../basicutils/bin/basicutils-config --cflags)

LINK_LIBS += $(shell ../bin/PixelStream-config --libs)
LINK_LIBS += $(shell ../../TaraDraw/bin/TaraDraw-config --libs)
LINK_LIBS += $(shell ../../basicutils/bin/basicutils-config --libs)

ifeq ($(OSSPEC), MacOS)
    CXXFLAGS += -I/opt/X11/include
    LINK_LIBS += -framework OpenGL -framework AppKit
else
    CXXFLAGS += -I/usr/X11R6/include
# add -lGLU below if NPOT (nearest-power-of-two) conversion is needed
    LINK_LIBS += -L/usr/X11R6/lib -lX11 -lXi -lXmu -lGL
endif

$(BINDIR)/PixelStreamReader: PixelStreamReader.cc
	mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LINK_LIBS)

clean:
	rm -rf $(BINDIR)

.PHONY: clean

OSSPEC := $(shell ../PixelStream-config --osspec)
BINDIR := $(shell ../PixelStream-config --bindir)

CXXFLAGS += -Wall
CXXFLAGS += $(shell ../PixelStream-config --cflags)
CXXFLAGS += $(shell ../../TaraDraw/TaraDraw-config --cflags)
CXXFLAGS += $(shell ../../utils/utils-config --cflags)

LINK_LIBS += $(shell ../PixelStream-config --libs)
LINK_LIBS += $(shell ../../TaraDraw/TaraDraw-config --libs)
LINK_LIBS += $(shell ../../utils/utils-config --libs)

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

.PHONY: all clean

OSSPEC := $(shell ../bin/TaraDraw-config --osspec)
BINDIR := $(shell ../bin/TaraDraw-config --bindir)

CXXFLAGS += -Wall
CXXFLAGS += $(shell ../bin/TaraDraw-config --cflags)

LINK_LIBS += $(shell ../bin/TaraDraw-config --libs)

ifeq ($(OSSPEC), MacOS)
    CXXFLAGS += -I/opt/X11/include
    LINK_LIBS += -framework OpenGL
else
    CXXFLAGS += -I/usr/X11R6/include
    LINK_LIBS += -L/usr/X11R6/lib -lX11 -lXi -lXmu -lGL -lGLU
endif

$(BINDIR)/td_demo: td_demo.cc
	mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LINK_LIBS)

clean:
	rm -rf $(BINDIR)

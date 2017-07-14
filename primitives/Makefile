.NOTPARALLEL:
.PHONY: all clean

OBJDIR := obj_MacOS
LIBDIR := lib_MacOS

HEADERS := $(wildcard *.hh)
OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(wildcard *.cc)), $(OBJDIR)/$(obj))

CXXFLAGS += -Wall $(shell ./bin/primitives-config --cflags_internal)

all: $(LIBDIR)/libprimitives.a

$(LIBDIR)/libprimitives.a: $(OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(OBJDIR)/%.o: %.cc $(HEADERS)
	mkdir -p $(OBJDIR)
#	$(CXX) $(CXXFLAGS) -c $< -o $@  TODO: fix with -config file
	$(CXX) $(CXXFLAGS) -I/Users/mmcfarla/data/packages/dcapp -I/Users/mmcfarla/data/packages/dcapp/packages -I/opt/X11/include/freetype2 -I/opt/X11/include -c $< -o $@ 

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)

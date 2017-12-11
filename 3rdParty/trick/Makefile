.PHONY: clean

OBJDIR := $(shell ./bin/TrickPlugin-config --objdir)
LIBDIR := $(shell ./bin/TrickPlugin-config --libdir)

HEADERS := $(wildcard *.hh)
SOURCES := trickcomm.cc

TRICK_HOME ?= $(or $(dir $(shell which trick-gte 2> /dev/null)), $(dir $(shell which gte 2> /dev/null)))
TRICK_HOME := $(TRICK_HOME:%/bin/=%)

ifneq (, $(TRICK_HOME))
    SOURCES += vscomm.cc
    CXXFLAGS += -DTRICKACTIVE
    ifneq (, $(wildcard $(TRICK_HOME)/bin/trick-gte))
        CXXFLAGS +=  -DTRICK16PLUS -I$(TRICK_HOME)/include
    else
        CXXFLAGS += -I$(TRICK_HOME)/trick_source
    endif
endif

OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(SOURCES)), $(OBJDIR)/$(obj))

CXXFLAGS += -std=c++11 -Wall -I../.. $(shell ../../packages/basicutils/bin/basicutils-config --cflags)

$(LIBDIR)/libTrickPlugin.a: $(OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(OBJDIR)/%.o: %.cc $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)

.PHONY: clean

OBJDIR := $(shell ./PkgInfo --objdir)
LIBDIR := $(shell ./PkgInfo --libdir)

HEADERS := $(wildcard *.hh)
SOURCES := trickcomm.cc
ifneq ($(shell which trick-gte 2> /dev/null),)
    SOURCES += vscomm.cc
    CXXFLAGS += -DTRICKACTIVE -DTRICK16PLUS -I$(shell trick-gte TRICK_HOME)/include
else ifneq ($(shell which gte 2> /dev/null),)
    SOURCES += vscomm.cc
    CXXFLAGS += -DTRICKACTIVE -I$(shell gte TRICK_HOME)/trick_source
endif
OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(SOURCES)), $(OBJDIR)/$(obj))

CXXFLAGS += -Wall -I../.. $(shell ../../packages/utils/PkgInfo --cflags)

$(LIBDIR)/libTrickComm.a: $(OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(OBJDIR)/%.o: %.cc $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)

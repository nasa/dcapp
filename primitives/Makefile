.PHONY: clean

OSSPEC := $(shell ./bin/primitives-config --osspec)
OBJDIR := $(shell ./bin/primitives-config --objdir)
LIBDIR := $(shell ./bin/primitives-config --libdir)

HEADERS := $(wildcard *.hh)
OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(wildcard *.cc)), $(OBJDIR)/$(obj))

CXXFLAGS += -std=c++11 -Wall -Wextra $(shell ./bin/primitives-config --cflags_internal)

SUBPACKAGE_CONFIGS := \
	../packages/PixelStream/bin/PixelStream-config \
	../packages/fontlib/bin/fontlib-config \
	../packages/basicutils/bin/basicutils-config

COMPDEPENDS := $(wildcard ../*.hh)
COMPDEPENDS += $(foreach subpackage, $(SUBPACKAGE_CONFIGS), $(shell $(subpackage) --compdepends))

$(LIBDIR)/libprimitives.a: $(OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(OBJDIR)/%.o: %.cc $(HEADERS) $(COMPDEPENDS)
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)

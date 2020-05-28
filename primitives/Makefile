.PHONY: clean

OBJDIR := $(shell ./bin/primitives-config --objdir)
LIBDIR := $(shell ./bin/primitives-config --libdir)

HEADERS := $(wildcard *.hh)
OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(wildcard *.cc)), $(OBJDIR)/$(obj))

SUBPACKAGE_CONFIGS := \
	../packages/PixelStream/bin/PixelStream-config \
	../packages/RenderLib/bin/RenderLib-config \
	../packages/basicutils/bin/basicutils-config

CXXFLAGS += -std=c++11 -Wall -Wextra $(shell ./bin/primitives-config --cflags_internal)
CXXFLAGS += $(foreach subpackage, $(SUBPACKAGE_CONFIGS), $(shell $(subpackage) --cflags))

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

.PHONY: clean

OBJDIR := $(shell ./bin/EdgePlugin-config --objdir)
LIBDIR := $(shell ./bin/EdgePlugin-config --libdir)

HEADERS := $(wildcard *.hh)
OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(wildcard *.cc)), $(OBJDIR)/$(obj))

CXXFLAGS += -std=c++11 -Wall -Wextra -I../.. $(shell ../../packages/basicutils/bin/basicutils-config --cflags)

$(LIBDIR)/libEdgePlugin.a: $(OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(OBJDIR)/%.o: %.cc $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)

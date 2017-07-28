.PHONY: clean

OSSPEC := $(shell ./bin/primitives-config --osspec)
OBJDIR := $(shell ./bin/primitives-config --objdir)
LIBDIR := $(shell ./bin/primitives-config --libdir)

HEADERS := $(wildcard *.hh)
OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(wildcard *.cc)), $(OBJDIR)/$(obj))

CXXFLAGS += -Wall $(shell ./bin/primitives-config --cflags_internal)

$(LIBDIR)/libprimitives.a: $(OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(OBJDIR)/%.o: %.cc $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)

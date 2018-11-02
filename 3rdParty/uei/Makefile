.PHONY: clean

OBJDIR := $(shell ./bin/UeiPlugin-config --objdir)
LIBDIR := $(shell ./bin/UeiPlugin-config --libdir)

HEADERS := $(wildcard *.hh)
OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(wildcard *.cc)), $(OBJDIR)/$(obj))

CXXFLAGS += -Wall -Wextra -I../.. $(shell ../../packages/basicutils/bin/basicutils-config --cflags)

$(LIBDIR)/libUeiPlugin.a: $(OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(OBJDIR)/%.o: %.cc $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)

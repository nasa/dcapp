.PHONY: clean

OBJDIR := $(shell ./bin/HagstromPlugin-config --objdir)
LIBDIR := $(shell ./bin/HagstromPlugin-config --libdir)

HEADERS := $(wildcard *.hh)
OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(wildcard *.cc)), $(OBJDIR)/$(obj))

CXXFLAGS += -Wall -Wextra $(shell ./bin/HagstromPlugin-config --cflags_internal)

$(LIBDIR)/libHagstromPlugin.a: $(OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(OBJDIR)/%.o: %.cc $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)

.PHONY: clean

OBJDIR := $(shell ../bin/TaraDraw-config --objdir)
LIBDIR := $(shell ../bin/TaraDraw-config --libdir)

HEADERS := $(wildcard *.hh)
OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(wildcard *.cc)), $(OBJDIR)/$(obj))

CXXFLAGS += -Wall -Wextra $(shell ../bin/TaraDraw-config --cflags_internal)

$(LIBDIR)/libTD.a: $(OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)

$(OBJDIR)/%.o: %.cc $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)

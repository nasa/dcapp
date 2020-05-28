.PHONY: clean

OBJDIR := $(shell ../bin/TaraDraw-config --objdir)
LIBDIR := $(shell ../bin/TaraDraw-config --libdir)

HEADERS := $(wildcard *.hh)
OBJECTS := $(foreach obj, $(patsubst %.mm, %.o, $(wildcard *.mm)), $(OBJDIR)/$(obj))

CXXFLAGS += -Wall -Wextra  $(shell ../bin/TaraDraw-config --cflags_internal)

$(LIBDIR)/libTD.a: $(OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)

$(OBJDIR)/%.o: %.mm $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)

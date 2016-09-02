.PHONY: clean

OBJDIR := $(shell ../TaraDraw-config --objdir)
LIBDIR := $(shell ../TaraDraw-config --libdir)

HEADERS := $(wildcard *.hh)
OBJECTS := $(foreach obj, $(patsubst %.mm, %.o, $(wildcard *.mm)), $(OBJDIR)/$(obj))

CXXFLAGS += -Wall

$(LIBDIR)/libTD.a: $(OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)

$(OBJDIR)/%.o: %.mm $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)

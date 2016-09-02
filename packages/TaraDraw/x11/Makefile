.PHONY: clean

OSSPEC := $(shell ../TaraDraw-config --osspec)
OBJDIR := $(shell ../TaraDraw-config --objdir)
LIBDIR := $(shell ../TaraDraw-config --libdir)

HEADERS := $(wildcard *.hh)
OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(wildcard *.cc)), $(OBJDIR)/$(obj))

CXXFLAGS += -Wall
ifeq ($(OSSPEC), MacOS)
    CXXFLAGS += -I/opt/X11/include
else
    CXXFLAGS += -I/usr/X11R6/include
endif

$(LIBDIR)/libTD.a: $(OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)

$(OBJDIR)/%.o: %.cc $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)

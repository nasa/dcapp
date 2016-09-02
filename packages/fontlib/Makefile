.PHONY: clean

OSSPEC := $(shell ./fontlib-config --osspec)
OBJDIR := $(shell ./fontlib-config --objdir)
LIBDIR := $(shell ./fontlib-config --libdir)

HEADERS := $(wildcard *.hh)
OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(wildcard *.cc)), $(OBJDIR)/$(obj))

CXXFLAGS += -Wall $(shell freetype-config --cflags)
ifeq ($(OSSPEC), MacOS)
    CXXFLAGS += -I/opt/X11/include
else
    CXXFLAGS += -I/usr/X11R6/include
endif

$(LIBDIR)/libfontlib.a: $(OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(OBJDIR)/%.o: %.cc $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)

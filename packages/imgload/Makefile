.PHONY: clean

OSSPEC := $(shell ./bin/imgload-config --osspec)
OBJDIR := $(shell ./bin/imgload-config --objdir)
LIBDIR := $(shell ./bin/imgload-config --libdir)

HEADERS := $(wildcard *.hh)
OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(wildcard *.cc)), $(OBJDIR)/$(obj))

CXXFLAGS += -Wall $(shell ../utils/bin/utils-config --cflags)
ifeq ($(OSSPEC), MacOS)
    CXXFLAGS += -I/opt/X11/include
else
    CXXFLAGS += -I/usr/X11R6/include
endif

$(LIBDIR)/libimgload.a: $(OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(OBJDIR)/%.o: %.cc $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)

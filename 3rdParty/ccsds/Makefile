.PHONY: clean

OBJDIR := $(shell ./bin/CcsdsPlugin-config --objdir)
LIBDIR := $(shell ./bin/CcsdsPlugin-config --libdir)

HEADERS := $(wildcard *.hh)
OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(wildcard *.cc)), $(OBJDIR)/$(obj))

CXXFLAGS += -Wall -I../.. $(shell ../../packages/basicutils/bin/basicutils-config --cflags)
ifdef CCSDS_UDP_HOME
    CXXFLAGS += -DCCSDSUDPACTIVE -I$(CCSDS_UDP_HOME)
endif

$(LIBDIR)/libCcsdsPlugin.a: $(OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(OBJDIR)/%.o: %.cc $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)

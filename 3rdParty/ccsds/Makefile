.PHONY: clean

OBJDIR := $(shell ./PkgInfo --objdir)
LIBDIR := $(shell ./PkgInfo --libdir)

HEADERS := $(wildcard *.hh)
OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(wildcard *.cc)), $(OBJDIR)/$(obj))

CXXFLAGS += -Wall -I../.. $(shell ../../packages/utils/PkgInfo --cflags)
ifdef CCSDS_UDP_HOME
    CXXFLAGS += -DCCSDSUDPACTIVE -I$(CCSDS_UDP_HOME)
endif

$(LIBDIR)/libCcsdsUdpComm.a: $(OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(OBJDIR)/%.o: %.cc $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)

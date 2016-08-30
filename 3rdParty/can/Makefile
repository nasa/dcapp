.PHONY: clean

OBJDIR := $(shell ./PkgInfo --objdir)
LIBDIR := $(shell ./PkgInfo --libdir)

HEADERS := $(wildcard *.hh)
OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(wildcard *.cc)), $(OBJDIR)/$(obj))

CXXFLAGS += -Wall $(shell ../../packages/utils/PkgInfo --cflags)
ifdef CANBUS_HOME
    CXXFLAGS += -DNTCAN -I$(CANBUS_HOME)
endif

$(LIBDIR)/libCAN.a: $(OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(OBJDIR)/%.o: %.cc $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)

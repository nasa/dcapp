.PHONY: clean

OSSPEC := $(shell ./bin/osenv-config --osspec)
OBJDIR := $(shell ./bin/osenv-config --objdir)
LIBDIR := $(shell ./bin/osenv-config --libdir)

HEADERS := $(wildcard *.hh)
ifeq ($(OSSPEC), MacOS)
    OBJECTS := $(foreach obj, $(patsubst %.mm, %.o, $(wildcard *.mm)), $(OBJDIR)/$(obj))
    CXXFLAGS += -Wno-deprecated -Wno-deprecated-declarations
else
    OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(wildcard *.cc)), $(OBJDIR)/$(obj))
endif
$(LIBDIR)/libosenv.a: $(OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

ifeq ($(OSSPEC), MacOS)
$(OBJDIR)/%.o: %.mm $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
else
$(OBJDIR)/%.o: %.cc $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
endif

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)

.PHONY: clean

OSSPEC := $(shell ./PkgInfo --osspec)
OBJDIR := $(shell ./PkgInfo --objdir)
LIBDIR := $(shell ./PkgInfo --libdir)

HEADERS := $(wildcard *.hh)
ifeq ($(OSSPEC), MacOS)
    OBJECTS := $(foreach obj, $(patsubst %.mm, %.o, $(wildcard *.mm)), $(OBJDIR)/$(obj))
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

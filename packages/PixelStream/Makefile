.NOTPARALLEL:
.PHONY: all postbuild clean

OBJDIR := $(shell ./PixelStream-config --objdir)
LIBDIR := $(shell ./PixelStream-config --libdir)

HEADERS := $(wildcard *.hh)
OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(wildcard *.cc)), $(OBJDIR)/$(obj))

CXXFLAGS += -Wall $(shell ../utils/utils-config --cflags)

all: $(LIBDIR)/libPixelStream.a postbuild

$(LIBDIR)/libPixelStream.a: $(OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(OBJDIR)/%.o: %.cc $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

postbuild:
	${MAKE} -C samples

clean:
	${MAKE} -C samples clean
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)
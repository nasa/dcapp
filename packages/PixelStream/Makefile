.NOTPARALLEL:
.PHONY: all postbuild clean

OBJDIR := $(shell ./bin/PixelStream-config --objdir)
LIBDIR := $(shell ./bin/PixelStream-config --libdir)

HEADERS := $(wildcard *.hh)
OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(wildcard *.cc)), $(OBJDIR)/$(obj))

CXXFLAGS += -Wall $(shell ../utils/bin/utils-config --cflags)

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

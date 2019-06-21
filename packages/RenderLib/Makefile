.PHONY: clean

OSSPEC := $(shell ./bin/RenderLib-config --osspec)
OBJDIR := $(shell ./bin/RenderLib-config --objdir)
LIBDIR := $(shell ./bin/RenderLib-config --libdir)

HEADERS := $(wildcard *.hh)
OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(wildcard *.cc)), $(OBJDIR)/$(obj))

CXXFLAGS += -Wall -Wextra $(shell ./bin/RenderLib-config --cflags_internal)
ifeq ($(OSSPEC), MacOS)
    CXXFLAGS += -I/opt/X11/include
else
    CXXFLAGS += -I/usr/X11R6/include
endif

$(LIBDIR)/libRenderLib.a: $(OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(OBJDIR)/%.o: %.cc $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)
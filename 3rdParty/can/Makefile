.PHONY: clean

OBJDIR := $(shell ./bin/CanPlugin-config --objdir)
LIBDIR := $(shell ./bin/CanPlugin-config --libdir)

HEADERS := $(wildcard *.hh)
OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(wildcard *.cc)), $(OBJDIR)/$(obj))

CXXFLAGS += -Wall -Wextra $(shell ../../packages/basicutils/bin/basicutils-config --cflags)
ifdef CANBUS_HOME
    CXXFLAGS += -DNTCAN -I$(CANBUS_HOME)
endif

$(LIBDIR)/libCanPlugin.a: $(OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(OBJDIR)/%.o: %.cc $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)

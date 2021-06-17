.PHONY: clean

OBJDIR := $(shell ./bin/logicutils-config --objdir)
LIBDIR := $(shell ./bin/logicutils-config --libdir)

HEADERS := $(wildcard *.hh)
OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(wildcard *.cc)), $(OBJDIR)/$(obj))

CXXFLAGS = -std=c++11 -fPIC -Wall -Wextra
ifeq ($(shell $(DCAPPCONFIG) --ostype), linux)
CXXFLAGS += -D_GNU_SOURCE
endif

$(LIBDIR)/liblogicutils.a: $(OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(OBJDIR)/%.o: %.cc $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)

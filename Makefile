.NOTPARALLEL:
.PHONY: all prebuild postbuild clean

OSSPEC := $(shell ./bin/dcapp-config --osspec)
OBJDIR := $(shell ./bin/dcapp-config --objdir)

BINDIR := dcapp.app/Contents/$(OSSPEC)

HEADERS := \
	$(wildcard *.hh)
DCAPP_SOURCES := \
	animation.cc \
	app_main.cc \
	comm.cc \
	constants.cc \
	handle_bezel.cc \
	handle_keyboard.cc \
	handle_mouse.cc \
	handle_utils.cc \
	logic_stubs.cc \
	string_utils.cc \
	tara_funcs.cc \
	varlist.cc \
	varlist_utils.cc \
	xml_parse.cc \
	xml_stringsub.cc \
	xml_utils.cc
GENHEADER_SOURCES := \
	dcapp_genheader.cc \
	xml_stringsub.cc \
	xml_utils.cc

SUBPACKAGE_CONFIGS := \
    osenv/bin/osenv-config \
    primitives/bin/primitives-config \
    3rdParty/can/bin/CanPlugin-config \
    3rdParty/edge/bin/EdgePlugin-config \
    3rdParty/trick/bin/TrickPlugin-config \
    3rdParty/uei/bin/UeiPlugin-config \
    packages/PixelStream/bin/PixelStream-config \
    packages/TaraDraw/bin/TaraDraw-config \
    packages/RenderLib/bin/RenderLib-config \
    packages/basicutils/bin/basicutils-config

CXXFLAGS += -std=c++11 -Wall -Wextra -I.

CXXFLAGS += $(foreach subpackage, $(SUBPACKAGE_CONFIGS), $(shell $(subpackage) --cflags))
LINK_LIBS += $(foreach subpackage, $(SUBPACKAGE_CONFIGS), $(shell $(subpackage) --libs))
COMPDEPENDS := $(foreach subpackage, $(SUBPACKAGE_CONFIGS), $(shell $(subpackage) --compdepends))
LINKDEPENDS := $(foreach subpackage, $(SUBPACKAGE_CONFIGS), $(shell $(subpackage) --linkdepends))

CXXFLAGS += $(shell xml2-config --cflags)
# STANKY KLUDGE FOR XCODE 8 ISSUE:
LINK_LIBS += $(shell xml2-config --exec-prefix=/usr --libs)
#LINK_LIBS += $(shell xml2-config --libs)

LINK_LIBS += -ldl

ifeq ($(OSSPEC), MacOS)
    LINK_LIBS += -framework AppKit
else
    CXXFLAGS += -D_GNU_SOURCE
    LINK_LIBS += -lrt -L/usr/X11R6/lib -lX11 -lXi -lXmu
endif

DCAPP_OBJECTS := $(DCAPP_SOURCES)
DCAPP_OBJECTS := $(foreach obj, $(DCAPP_OBJECTS:.cc=.o), $(obj))
DCAPP_OBJECTS := $(foreach obj, $(patsubst %, $(OBJDIR)/%, $(DCAPP_OBJECTS)), $(obj))
GENHEADER_OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(GENHEADER_SOURCES)), $(OBJDIR)/$(obj))

#CXXFLAGS += -DDEBUG

all: prebuild $(BINDIR)/dcapp $(BINDIR)/dcapp_genheader postbuild

$(BINDIR)/dcapp: $(DCAPP_OBJECTS) $(LINKDEPENDS)
	mkdir -p $(BINDIR)
	$(CXX) $(LDFLAGS) $^ $(LINK_LIBS) -o $@

$(BINDIR)/dcapp_genheader: $(GENHEADER_OBJECTS) $(LINKDEPENDS)
	mkdir -p $(BINDIR)
	$(CXX) $(LDFLAGS) $^ $(LINK_LIBS) -o $@

$(OBJDIR)/%.o: %.cc $(HEADERS) $(COMPDEPENDS)
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

prebuild:
	${MAKE} -C packages
	${MAKE} -C 3rdParty
	${MAKE} -C osenv
	${MAKE} -C primitives

postbuild:
	${MAKE} -C samples

clean:
	${MAKE} -C packages clean
	${MAKE} -C 3rdParty clean
	${MAKE} -C osenv clean
	${MAKE} -C primitives clean
	${MAKE} -C samples clean
	rm -rf $(OBJDIR)
	rm -rf $(BINDIR)

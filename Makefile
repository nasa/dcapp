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
	geometry.cc \
	handle_bezel.cc \
	handle_draw.cc \
	handle_keyboard.cc \
	handle_mouse.cc \
	handle_streams.cc \
	handle_utils.cc \
	loadUtils.cc \
	logic_stubs.cc \
	opengl_draw.cc \
	primitive_new.cc \
	render_ADI.cc \
	render_string.cc \
	string_utils.cc \
	tara_funcs.cc \
	update_display.cc \
	varlist.cc \
	xml_parse.cc \
	xml_stringsub.cc \
	xml_utils.cc
GENHEADER_SOURCES := \
	dcapp_genheader.cc \
	xml_stringsub.cc \
	xml_utils.cc

SUBPACKAGE_CONFIGS := \
    osenv/bin/osenv-config \
    3rdParty/can/bin/CanPlugin-config \
    3rdParty/ccsds/bin/CcsdsPlugin-config \
    3rdParty/edge/bin/EdgePlugin-config \
    3rdParty/trick/bin/TrickPlugin-config \
    3rdParty/uei/bin/UeiPlugin-config \
    packages/PixelStream/bin/PixelStream-config \
    packages/TaraDraw/bin/TaraDraw-config \
    packages/fontlib/bin/fontlib-config \
    packages/imgload/bin/imgload-config \
    packages/basicutils/bin/basicutils-config

CXXFLAGS += -Wall -I.

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
    CXXFLAGS += -I/opt/X11/include
    LINK_LIBS += -framework OpenGL -framework AppKit
else
    CXXFLAGS += -D_GNU_SOURCE -I/usr/X11R6/include
    LINK_LIBS += -lrt -L/usr/X11R6/lib -lX11 -lXi -lXmu -lGL -lGLU
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

postbuild:
	${MAKE} -C samples

clean:
	${MAKE} -C packages clean
	${MAKE} -C 3rdParty clean
	${MAKE} -C osenv clean
	${MAKE} -C samples clean
	rm -rf $(OBJDIR)
	rm -rf $(BINDIR)

.NOTPARALLEL:
.PHONY: all prebuild postbuild clean

OSSPEC := $(shell ./PkgInfo --osspec)
OBJDIR := $(shell ./PkgInfo --objdir)

BINDIR := dcapp.app/Contents/$(OSSPEC)

HEADERS := \
	$(wildcard *.hh)
DCAPP_SOURCES := \
	PixelStreamData.cc \
	PixelStreamFile.cc \
	PixelStreamTcp.cc \
	animation.cc \
	app_main.cc \
	comm.cc \
	geometry.cc \
	handle_bezel.cc \
	handle_draw.cc \
	handle_keyboard.cc \
	handle_mouse.cc \
	handle_utils.cc \
	loadUtils.cc \
	logic_stubs.cc \
	nodes.cc \
	opengl_draw.cc \
	primitive_new.cc \
	render_ADI.cc \
	render_string.cc \
	string_utils.cc \
	tara_funcs.cc \
	update_display.cc \
	varlist.cc \
	xml_parse.cc \
	xml_utils.cc
GENHEADER_SOURCES := \
	dcapp_genheader.cc \
	xml_utils.cc

CXXFLAGS += -Wall -I.
CXXFLAGS += $(shell osenv/PkgInfo --cflags)
CXXFLAGS += $(shell 3rdParty/can/PkgInfo --cflags)
CXXFLAGS += $(shell 3rdParty/ccsds/PkgInfo --cflags)
CXXFLAGS += $(shell 3rdParty/edge/PkgInfo --cflags)
CXXFLAGS += $(shell 3rdParty/trick/PkgInfo --cflags)
CXXFLAGS += $(shell 3rdParty/uei/PkgInfo --cflags)
CXXFLAGS += $(shell packages/TaraDraw/PkgInfo --cflags)
CXXFLAGS += $(shell packages/fontlib/PkgInfo --cflags)
CXXFLAGS += $(shell packages/imgload/PkgInfo --cflags)
CXXFLAGS += $(shell packages/utils/PkgInfo --cflags)
CXXFLAGS += $(shell xml2-config --cflags)

LINK_LIBS += $(shell osenv/PkgInfo --libs)
LINK_LIBS += $(shell 3rdParty/can/PkgInfo --libs)
LINK_LIBS += $(shell 3rdParty/ccsds/PkgInfo --libs)
LINK_LIBS += $(shell 3rdParty/edge/PkgInfo --libs)
LINK_LIBS += $(shell 3rdParty/trick/PkgInfo --libs)
LINK_LIBS += $(shell 3rdParty/uei/PkgInfo --libs)
LINK_LIBS += $(shell packages/TaraDraw/PkgInfo --libs)
LINK_LIBS += $(shell packages/fontlib/PkgInfo --libs)
LINK_LIBS += $(shell packages/imgload/PkgInfo --libs)
LINK_LIBS += $(shell packages/utils/PkgInfo --libs)
LINK_LIBS += $(shell xml2-config --libs)
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

$(BINDIR)/dcapp: $(DCAPP_OBJECTS)
	mkdir -p $(BINDIR)
	$(CXX) $(LDFLAGS) $^ $(LINK_LIBS) -o $@

$(BINDIR)/dcapp_genheader: $(GENHEADER_OBJECTS)
	mkdir -p $(BINDIR)
	$(CXX) $(LDFLAGS) $^ $(LINK_LIBS) -o $@

$(OBJDIR)/%.o: %.cc $(HEADERS)
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

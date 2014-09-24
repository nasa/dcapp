.NOTPARALLEL:

include makedefs

HEADERS := \
	$(wildcard *.hh) \
	$(wildcard imgload/*.hh) \
	$(wildcard uei/*.hh) \
	$(wildcard TaraDraw/*.hh)
DCAPP_SOURCES := \
	CAN.cc \
	EDGE_rcs.cc \
	app_main.cc \
	ccsds_udp_io.cc \
	dyn_elements.cc \
	edgeio.cc \
	geometry.cc \
	handle_bezel.cc \
	handle_draw.cc \
	handle_keyboard.cc \
	handle_mouse.cc \
	handle_utils.cc \
	load_constants.cc \
	load_fonts.cc \
	load_shm.cc \
	load_textures.cc \
	logic_stubs.cc \
	nodes.cc \
	opengl_draw.cc \
	primitive_new.cc \
	render_ADI.cc \
	render_string.cc \
	simio.cc \
	string_utils.cc \
	trickio.cc \
	update_display.cc \
	xml_parse.cc \
	xml_utils.cc
GENHEADER_SOURCES := \
	dcapp_genheader.cc \
	xml_utils.cc
LIBS := \
	uei/$(LIBDIR)/libuei.a \
	imgload/$(LIBDIR)/libimgload.a

ifeq ($(UseFTGL), yes)
FTGL_CFLAG += -DUseFTGL
FTGL_LFLAG += -lftgl
else
DCAPP_SOURCES += fontlib.cc
endif

ifeq ($(shell uname), Linux)
TARA_SUBDIR := x11
ifeq ($(UseGLUT), yes)
DCAPP_SOURCES += glut_funcs.cc app_launcher_stub.cc
UI_CFLAG := -I../glut/include
UI_LFLAG := -L../glut/lib/glut -lglut
else
DCAPP_SOURCES += tara_funcs.cc app_launcher_stub.cc
UI_CFLAG :=
UI_LFLAG := -LTaraDraw/$(TARA_SUBDIR)/$(LIBDIR) -lTD
LIBS += TaraDraw/$(TARA_SUBDIR)/$(LIBDIR)/libTD.a
endif
UI_CFLAG += -I/usr/X11R6/include
UI_LFLAG += -L/usr/X11R6/lib -lX11 -lXi -lXmu -lGL -lGLU
COPY_SCRIPTS :=
else
TARA_SUBDIR := mac
ifeq ($(UseGLUT), yes)
DCAPP_SOURCES += glut_funcs.cc app_launcher_stub.cc
UI_CFLAG := -I/usr/X11R6/include
UI_LFLAG := -L/usr/X11R6/lib -lX11 -lXi -lXmu -lGL -lGLU -lglut
COPY_SCRIPTS :=
else
DCAPP_SOURCES += tara_funcs.cc app_launcher.mm
UI_CFLAG := -I/usr/X11R6/include
UI_LFLAG := -LTaraDraw/$(TARA_SUBDIR)/$(LIBDIR) -lTD -framework OpenGL -framework AppKit
COPY_SCRIPTS := mv -f $(BINDIR)/dcapp dcapp.app/Contents/MacOS; cp -f bin/launcher.py $(BINDIR)/dcapp
LIBS += TaraDraw/$(TARA_SUBDIR)/$(LIBDIR)/libTD.a
endif
endif

ifdef TRICK_HOME
#TRICK_MAJOR = $(word 1,$(subst ., ,$(TRICK_VER)))
DCAPP_SOURCES += vscomm.cc
TRICK_CFLAG := -DTRICKACTIVE -I$(TRICK_HOME)/trick_source
TRICK_LFLAG := -L$(TRICK_HOME)/trick_source/trick_utils/comm/object_$(TRICK_HOST_TYPE) -ltrick_comm
endif

ifdef CANBUS_HOME
CAN_CFLAG := -DNTCAN -I$(CANBUS_HOME)
CAN_LFLAG := -L$(CANBUS_HOME) -Wl,-Bstatic -lntcan -Wl,-Bdynamic
endif

ifdef CCSDS_UDP_HOME
CCSDS_UDP_CFLAG := -DCCSDSUDPACTIVE -I$(CCSDS_UDP_HOME)
# -rdynamic is needed for GCC to use the application's symbols to resolve undefined symbols in the loaded .so's
CCSDS_UDP_LFLAG := -rdynamic -L$(CCSDS_UDP_HOME)/Debug -lccsds_udp
endif

FT_CFLAG := $(shell freetype-config --cflags)
FT_LFLAG := $(shell freetype-config --libs)

XML2_CFLAG := $(shell xml2-config --cflags)
XML2_LFLAG := $(shell xml2-config --libs)

COMP_FLAGS := $(XML2_CFLAG) $(FTGL_CFLAG) $(FT_CFLAG) $(UI_CFLAG) $(CAN_CFLAG) $(TRICK_CFLAG) $(CCSDS_UDP_CFLAG)
LINK_FLAGS := $(XML2_LFLAG) $(FTGL_LFLAG) $(FT_LFLAG) $(UI_LFLAG) $(CAN_LFLAG) $(TRICK_LFLAG) $(CCSDS_UDP_LFLAG) -ldl

ifeq ($(shell uname), Linux)
COMP_FLAGS += -D_GNU_SOURCE
LINK_FLAGS += -lrt
endif

DCAPP_OBJECTS := $(DCAPP_SOURCES)
DCAPP_OBJECTS := $(foreach obj, $(DCAPP_OBJECTS:.cc=.o), $(obj))
DCAPP_OBJECTS := $(foreach obj, $(DCAPP_OBJECTS:.mm=.o), $(obj))
DCAPP_OBJECTS := $(foreach obj, $(patsubst %, $(OBJDIR)/%, $(DCAPP_OBJECTS)), $(obj))
GENHEADER_OBJECTS := $(foreach obj, $(patsubst %.cc, %.o, $(GENHEADER_SOURCES)), $(OBJDIR)/$(obj))

#COMP_FLAGS += -DDEBUG

dcapp: $(BINDIR)/dcapp $(BINDIR)/dcapp_genheader

$(BINDIR)/dcapp: $(DCAPP_OBJECTS) $(LIBS)
	mkdir -p $(BINDIR)
	$(LD) $(LDFLAGS) $^ -o $@ $(LINK_FLAGS)
	$(COPY_SCRIPTS)

$(BINDIR)/dcapp_genheader: $(GENHEADER_OBJECTS)
	mkdir -p $(BINDIR)
	$(LD) $(LDFLAGS) $^ -o $@ $(LINK_FLAGS)

$(OBJDIR)/%.o: %.cc $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) $(COMP_FLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.mm $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) $(COMP_FLAGS) -c $< -o $@

uei/$(LIBDIR)/libuei.a: $(wildcard uei/*.cc) $(wildcard uei/*.hh)
	make -C uei

imgload/$(LIBDIR)/libimgload.a: $(wildcard imgload/*.cc) $(wildcard imgload/*.hh)
	make -C imgload

TaraDraw/$(TARA_SUBDIR)/$(LIBDIR)/libTD.a: $(wildcard TaraDraw/$(TARA_SUBDIR)/*.mm) $(wildcard TaraDraw/$(TARA_SUBDIR)/*.cc) $(wildcard TaraDraw/$(TARA_SUBDIR)/*.hh)
	make -C TaraDraw/$(TARA_SUBDIR)

clean:
	make -C uei clean
	make -C imgload clean
	make -C TaraDraw/$(TARA_SUBDIR) clean
	rm -rf $(OBJDIR)
	rm -rf $(BINDIR)
	rm -f dcapp.app/Contents/MacOS/dcapp

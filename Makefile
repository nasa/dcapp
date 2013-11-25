.NOTPARALLEL:

include makedefs

HEADERS := \
	$(wildcard *.h) \
	$(wildcard imgload/*.h) \
	$(wildcard uei/*.h) \
	$(wildcard TaraDraw/*.h)
DCAPP_SOURCES := \
	CAN.c \
	EDGE_rcs.c \
	app_main.c \
	dyn_elements.c \
	edgeio.c \
	geometry.c \
	handle_bezel.c \
	handle_draw.c \
	handle_keyboard.c \
	handle_mouse.c \
	handle_utils.c \
	load_constants.c \
	load_fonts.c \
	load_shm.c \
	load_textures.c \
	logic_stubs.c \
	nodes.c \
	opengl_draw.c \
	primitive_new.c \
	render_ADI.c \
	render_string.c \
	simio.c \
	string_utils.c \
	trickio.c \
	update_display.c \
	vscomm.c \
	xml_parse.c \
	xml_utils.c
GENHEADER_SOURCES := \
	dcapp_genheader.c \
	xml_utils.c
LIBS := \
	uei/$(LIBDIR)/libuei.a \
	imgload/$(LIBDIR)/libimgload.a

ifeq ($(UseFTGL), yes)
FTGL_CFLAG += -DUseFTGL
FTGL_LFLAG += -lftgl
else
DCAPP_SOURCES += fontlib.c
endif

ifeq ($(shell uname), Linux)
TARA_SUBDIR := x11
ifeq ($(UseGLUT), yes)
DCAPP_SOURCES += glut_funcs.c app_launcher_stub.c
UI_CFLAG := -I../glut/include
UI_LFLAG := -L../glut/lib/glut -lglut
else
DCAPP_SOURCES += tara_funcs.c app_launcher_stub.c
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
DCAPP_SOURCES += glut_funcs.c app_launcher_stub.c
UI_CFLAG := -I/usr/X11R6/include
UI_LFLAG := -L/usr/X11R6/lib -lX11 -lXi -lXmu -lGL -lGLU -lglut
COPY_SCRIPTS :=
else
DCAPP_SOURCES += tara_funcs.c app_launcher.m
UI_CFLAG :=
UI_LFLAG := -LTaraDraw/$(TARA_SUBDIR)/$(LIBDIR) -lTD -framework OpenGL -framework AppKit
COPY_SCRIPTS := mv -f $(BINDIR)/dcapp dcapp.app/Contents/MacOS; cp -f bin/launcher.py $(BINDIR)/dcapp
LIBS += TaraDraw/$(TARA_SUBDIR)/$(LIBDIR)/libTD.a
endif
endif

DCAPP_OBJECTS := $(DCAPP_SOURCES)
DCAPP_OBJECTS := $(foreach obj, $(DCAPP_OBJECTS:.c=.o), $(obj))
DCAPP_OBJECTS := $(foreach obj, $(DCAPP_OBJECTS:.m=.o), $(obj))
DCAPP_OBJECTS := $(foreach obj, $(patsubst %, $(OBJDIR)/%, $(DCAPP_OBJECTS)), $(obj))
GENHEADER_OBJECTS := $(foreach obj, $(patsubst %.c, %.o, $(GENHEADER_SOURCES)), $(OBJDIR)/$(obj))

#TRICK_MAJOR = $(word 1,$(subst ., ,$(TRICK_VER)))
TRICK_CFLAG := -I$(TRICK_HOME)/trick_source
TRICK_LFLAG := -L$(TRICK_HOME)/trick_source/trick_utils/comm/object_$(TRICK_HOST_TYPE) -ltrick_comm

FT_CFLAG := $(shell freetype-config --cflags)
FT_LFLAG := $(shell freetype-config --libs)

XML2_CFLAG := $(shell xml2-config --cflags)
XML2_LFLAG := $(shell xml2-config --libs)

ifdef CANBUS_HOME
CAN_CFLAG := -DNTCAN -I$(CANBUS_HOME)
CAN_LFLAG := -L$(CANBUS_HOME) -Wl,-Bstatic -lntcan -Wl,-Bdynamic
endif

COMP_FLAGS := $(XML2_CFLAG) $(FTGL_CFLAG) $(FT_CFLAG) $(UI_CFLAG) $(CAN_CFLAG) $(TRICK_CFLAG)
LINK_FLAGS := $(XML2_LFLAG) $(FTGL_LFLAG) $(FT_LFLAG) $(UI_LFLAG) $(CAN_LFLAG) $(TRICK_LFLAG) -ldl

ifeq ($(shell uname), Linux)
COMP_FLAGS += -D_GNU_SOURCE
LINK_FLAGS += -lrt
endif

#COMP_FLAGS += -DDEBUG

dcapp: $(BINDIR)/dcapp $(BINDIR)/dcapp_genheader

$(BINDIR)/dcapp: $(DCAPP_OBJECTS) $(LIBS)
	mkdir -p $(BINDIR)
	$(LD) $(LDFLAGS) $^ -o $@ $(LINK_FLAGS)
	$(COPY_SCRIPTS)

$(BINDIR)/dcapp_genheader: $(GENHEADER_OBJECTS)
	mkdir -p $(BINDIR)
	$(LD) $(LDFLAGS) $^ -o $@ $(LINK_FLAGS)

$(OBJDIR)/%.o: %.c $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) $(COMP_FLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.m $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) $(COMP_FLAGS) -c $< -o $@

uei/$(LIBDIR)/libuei.a: $(wildcard uei/*.c) $(wildcard uei/*.h)
	make -C uei

imgload/$(LIBDIR)/libimgload.a: $(wildcard imgload/*.c) $(wildcard imgload/*.h)
	make -C imgload

TaraDraw/$(TARA_SUBDIR)/$(LIBDIR)/libTD.a: $(wildcard TaraDraw/$(TARA_SUBDIR)/*.m) $(wildcard TaraDraw/$(TARA_SUBDIR)/*.c) $(wildcard TaraDraw/$(TARA_SUBDIR)/*.h)
	make -C TaraDraw/$(TARA_SUBDIR)

clean:
	make -C uei clean
	make -C imgload clean
	make -C TaraDraw/$(TARA_SUBDIR) clean
	rm -rf $(OBJDIR)
	rm -rf $(BINDIR)
	rm -f dcapp.app/Contents/MacOS/dcapp

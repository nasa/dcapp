.PHONY: clean

DCAPP_HOME ?= $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../..)
BASIC_UTILS ?= ${DCAPP_HOME}/packages/logicutils
PARAM_FILE := ../blinker.xml

ifneq ($(wildcard ${DCAPP_HOME}/dcapp.app/Contents/dcapp-config),)
	DCAPPCONFIG := ${DCAPP_HOME}/dcapp.app/Contents/dcapp-config
else
	DCAPPCONFIG := dcapp-config
endif

OBJDIR := $(shell $(DCAPPCONFIG) --objdir)
EXEPATH := $(shell ${DCAPPCONFIG} --exepath)
LIBDIR := $(shell ${DCAPPCONFIG} --libdir)

ifneq ($(wildcard ${EXEPATH}/dcapp_genheader),)
	GENHEADER := ${EXEPATH}/dcapp_genheader
else
	GENHEADER := dcapp_genheader
endif

# ----------------------------------------------------------------------------------------
SOURCES := \
	mylogic.cc

HEADERS := \
	${BASIC_UTILS}/blinker.hh

OBJECTS := $(foreach obj, $(SOURCES:.cc=.o), $(dir $(obj))$(OBJDIR)/$(notdir $(obj)))

CXXFLAGS += -fPIC -Wall -std=c++11
ifeq ($(shell $(DCAPPCONFIG) --ostype), linux)
CXXFLAGS += -D_GNU_SOURCE
endif
CXXFLAGS += -I.
CXXFLAGS += $(sort $(foreach header, $(HEADERS), -I$(dir $(header))))

# ----------------------------------------------------------------------------------------

all: dcapp.hh mylogic.so

dcapp.hh: ${PARAM_FILE}
	${GENHEADER} ${PARAM_FILE} $@

mylogic.so: ${OBJECTS}  ${BASIC_UTILS}/$(LIBDIR)/liblogicutils.so
	${CXX} -Wall -shared $^ -o $@

define BUILDOBJECT
$(1): $(realpath $(dir $(1))../$(basename $(notdir $(1))).cc) dcapp.hh ${HEADERS}
	mkdir -p $(dir $(1))
	${CXX} -c ${CXXFLAGS} $(dir $(1))../$(basename $(notdir $(1))).cc -o $(1)
endef
$(foreach object, $(OBJECTS), $(eval $(call BUILDOBJECT, $(object))))

clean:
	rm -f dcapp.hh ${OBJECTS} mylogic.so

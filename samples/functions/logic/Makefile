.PHONY: clean

DCAPP_HOME ?= $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../..)

ifneq ($(wildcard ${DCAPP_HOME}/dcapp.app/Contents/dcapp-config),)
    DCAPPCONFIG := ${DCAPP_HOME}/dcapp.app/Contents/dcapp-config
else
    DCAPPCONFIG := dcapp-config
endif

EXEPATH := $(shell ${DCAPPCONFIG} --exepath)
OBJDIR := $(shell $(DCAPPCONFIG) --objdir)

SOURCES := \
    mylogic.cc \
    ../smiley/smiley_logic.cc
OBJECTS := $(foreach obj, $(SOURCES:.cc=.o), $(OBJDIR)/$(notdir $(obj)))

CXXFLAGS += -fPIC -Wall -std=c++11
ifeq ($(shell $(DCAPPCONFIG) --ostype), linux)
CXXFLAGS += -D_GNU_SOURCE
endif

ifneq ($(wildcard ${EXEPATH}/dcapp_genheader),)
    GENHEADER := ${EXEPATH}/dcapp_genheader
else
    GENHEADER := dcapp_genheader
endif

define genObjectRule
$(OBJDIR)/$(basename $(notdir $(1))).o: dcapp.h $(1)
	mkdir -p $(OBJDIR)
	${CXX} -c ${CXXFLAGS} $(1) -o $(OBJDIR)/$(basename $(notdir $(1))).o
endef

all: mylogic.so

mylogic.so: ${OBJECTS}
	
	${GENHEADER} ../functions.xml
	${CXX} -Wall -Wextra -shared $^ -o $@

$(foreach source, $(SOURCES), $(eval $(call genObjectRule, $(source))))

dcapp.h: ../functions.xml
	${GENHEADER} ../functions.xml $@

clean:
	${RM} -rf dcapp.h mylogic.so $(OBJDIR)

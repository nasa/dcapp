.PHONY: clean

DCAPP_HOME ?= $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../..)

ifneq ($(wildcard ${DCAPP_HOME}/dcapp.app/Contents/dcapp-config),)
    DCAPPCONFIG := ${DCAPP_HOME}/dcapp.app/Contents/dcapp-config
else
    DCAPPCONFIG := dcapp-config
endif

EXEPATH := $(shell ${DCAPPCONFIG} --exepath)

ifneq ($(wildcard ${EXEPATH}/dcapp_genheader),)
    GENHEADER := ${EXEPATH}/dcapp_genheader
else
    GENHEADER := dcapp_genheader
endif

mylogic.so: mylogic.cc ../ptz.xml
	${GENHEADER} ../ptz.xml
	${CXX} -fPIC -Wall -Wextra -shared $< -o $@

clean:
	${RM} dcapp.h mylogic.so

.PHONY: clean

DCAPP_HOME ?= $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../..)

ifneq ($(wildcard ${DCAPP_HOME}/dcapp.app/Contents/dcapp_genheader),)
    GENHEADER := ${DCAPP_HOME}/dcapp.app/Contents/dcapp_genheader
else
    GENHEADER := dcapp_genheader
endif

mylogic.so: mylogic.cc ../screensaver.xml
	${GENHEADER} ../screensaver.xml
	${CXX} -fPIC -Wall -shared $< -o $@

clean:
	${RM} dcapp.h mylogic.so

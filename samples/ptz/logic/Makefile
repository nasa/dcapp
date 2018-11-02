.PHONY: clean

DCAPP_HOME ?= $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../..)

ifneq ($(wildcard ${DCAPP_HOME}/dcapp.app/Contents/dcapp_genheader),)
    GENHEADER := ${DCAPP_HOME}/dcapp.app/Contents/dcapp_genheader
else
    GENHEADER := dcapp_genheader
endif

mylogic.so: mylogic.cc ../ptz.xml
	${GENHEADER} ../ptz.xml
	${CXX} -fPIC -Wall -Wextra -shared $< -o $@

clean:
	${RM} dcapp.h mylogic.so

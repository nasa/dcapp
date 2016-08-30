.PHONY: clean

ifneq ($(wildcard ${DCAPP_HOME}/dcapp.app/Contents/dcapp_genheader),)
    GENHEADER := ${DCAPP_HOME}/dcapp.app/Contents/dcapp_genheader
else
    GENHEADER := dcapp_genheader
endif

mylogic.so: mylogic.cc ../module.xml
	${GENHEADER} ../module.xml
	${CXX} -fPIC -Wall -shared $< -o $@

clean:
	${RM} dcapp.h mylogic.so

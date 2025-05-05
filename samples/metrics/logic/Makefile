DCAPP_HOME := $(realpath $(CURDIR)/../../..)
DCAPP_GENHEADER := $(DCAPP_HOME)/bin/dcapp-genheader
CPPFLAGS := -std=c++17 -g -O2 -fPIC


all: logic.so

dcapp.h: ../metrics.xml
	$(DCAPP_GENHEADER) ../metrics.xml

logic.so: logic.cpp
	${CXX} $(CPPFLAGS) -shared $< -o $@

logic.cpp: dcapp.h

clean:
	${RM} dcapp.h logic.so

.PHONY: clean

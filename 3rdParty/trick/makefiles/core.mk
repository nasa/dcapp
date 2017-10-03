#=============================================================================
# Trick-compatible makefile for dcapp
#
# This file is intended for inclusion in a Trick S_overrides.mk file.
#
# It responds to the Trick-standard "all" and "spotless" targets, and it
# provides several other useful targets as well, including build_externals.
#
# It builds the dcapp executable and display logic shared objects, and it
# creates links to these, along with a link to the start_dcapp.py script,
# within an externals directory that is easily accesible by the Trick
# simulation.
#
# Note that the user may explicitly set DCAPP_HOME and/or DISPLAYS_HOME before
# using this file.  If not, it will set DCAPP_HOME based on a relative
# location from this file, and it will set DISPLAYS_HOME to
# $DCAPP_HOME/../displays.
#=============================================================================

export DCAPP_HOME ?= $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../..)
DISPLAYS_HOME ?= $(abspath $(DCAPP_HOME)/../displays)
EXTERNALS_DIR ?= $(abspath externals/dcapp)

all: build_dcapp build_dcapp_link

spotless: clean_dcapp clean_dcapp_link

build_externals: build_dcapp_link

build_dcapp:
	@ echo "[32mbuilding dcapp executable[00m"
	${MAKE} -C ${DCAPP_HOME}
	@ echo "[32mbuilding logic shared object(s) for dcapp display(s)[00m"
	${MAKE} -C ${DISPLAYS_HOME}

clean_dcapp:
	@ echo "[32mcleaning dcapp executable[00m"
	${MAKE} -C ${DCAPP_HOME} clean
	@ echo "[32mcleaning logic shared object(s) for dcapp display(s)[00m"
	${MAKE} -C ${DISPLAYS_HOME} clean

build_dcapp_link: clean_dcapp_link
	@ echo "[32mcreating links to dcapp files in externals[00m"
	@ if [ ! -e ${EXTERNALS_DIR} ] ; then mkdir -p ${EXTERNALS_DIR} ; fi 
	ln -s -f ${DCAPP_HOME}/bin ${EXTERNALS_DIR}
	ln -s -f ${DCAPP_HOME}/dcapp.app ${EXTERNALS_DIR}
	ln -s -f ${DCAPP_HOME}/3rdParty/trick/scripts/start_dcapp.py ${EXTERNALS_DIR}
	ln -s -f ${DISPLAYS_HOME} ${EXTERNALS_DIR}

clean_dcapp_link:
	@ echo "[32mcleaning links to dcapp files in externals[00m"
	rm -rf ${EXTERNALS_DIR}

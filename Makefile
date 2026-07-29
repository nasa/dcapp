# Stable GNU Make interface for dcapp consumers.
#
# The platform-specific build and tool wrappers remain the source of truth.
# This file gives parent repositories target names that do not change between
# macOS, Linux, and Windows.

CONFIG ?= release

OUT_DIR := $(CURDIR)/pilotlight/out

ifeq ($(OS),Windows_NT)
PLATFORM := win32
EXE_EXT := .exe
SHARED_LIB_PREFIX :=
SHARED_LIB_EXT := .dll
BUILD_COMMAND := cmd /C scripts\build.bat -c $(CONFIG)
GENHEADER_COMMAND := cmd /C bin\dcapp-genheader.bat
VALIDATE_COMMAND := cmd /C bin\dcapp-validate.bat
else
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
PLATFORM := macos
SHARED_LIB_EXT := .dylib
else
PLATFORM := linux
SHARED_LIB_EXT := .so
endif
EXE_EXT :=
SHARED_LIB_PREFIX := lib
BUILD_COMMAND := ./scripts/build.sh -c $(CONFIG)
GENHEADER_COMMAND := ./bin/dcapp-genheader.sh
VALIDATE_COMMAND := ./bin/dcapp-validate.sh
endif

BUILD_STAMP := $(OUT_DIR)/.dcapp-output-$(PLATFORM)-$(CONFIG).stamp
GENHEADER := $(OUT_DIR)/dcapp-genheader$(EXE_EXT)
VALIDATOR := $(OUT_DIR)/dcapp-validate$(EXE_EXT)
DCAPP_LIBRARY := $(OUT_DIR)/$(SHARED_LIB_PREFIX)dcapp$(SHARED_LIB_EXT)

.DEFAULT_GOAL := build

.PHONY: all build force-build genheader validate help
.PHONY: print-build-stamp print-genheader print-validator print-dcapp-library

all: build

## Build dcapp if its existing incremental build state is stale.
build:
	$(BUILD_COMMAND)

## Rebuild dcapp regardless of its existing incremental build state.
force-build:
	$(BUILD_COMMAND) -f

## Generate logic/dcapp.h. Usage: make genheader XML=/path/to/display.xml [ARGS="..."]
genheader: build
	$(if $(strip $(XML)),,$(error XML is required; use: make genheader XML=/path/to/display.xml))
	$(GENHEADER_COMMAND) "$(abspath $(XML))" $(ARGS)

## Validate an XML display. Usage: make validate XML=/path/to/display.xml [ARGS="..."]
validate: build
	$(if $(strip $(XML)),,$(error XML is required; use: make validate XML=/path/to/display.xml))
	$(VALIDATE_COMMAND) "$(abspath $(XML))" $(ARGS)

# These quiet query targets let parent Makefiles discover real files without
# duplicating dcapp's platform-specific filename rules.
print-build-stamp:
	@echo "$(BUILD_STAMP)"

print-genheader:
	@echo "$(GENHEADER)"

print-validator:
	@echo "$(VALIDATOR)"

print-dcapp-library:
	@echo "$(DCAPP_LIBRARY)"

help:
	@echo "dcapp Make targets:"
	@echo "  build / all                   Build only when dcapp is stale (default)"
	@echo "  force-build                   Force a complete dcapp rebuild"
	@echo "  genheader XML=<file> [ARGS=]  Generate the display logic header"
	@echo "  validate  XML=<file> [ARGS=]  Validate a display"
	@echo "  print-build-stamp             Print the real successful-build stamp"
	@echo "  print-genheader               Print the generator executable path"
	@echo "  print-validator               Print the validator executable path"
	@echo "  print-dcapp-library           Print the dcapp shared-library path"

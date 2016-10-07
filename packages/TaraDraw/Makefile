.PHONY: all clean

OSSPEC := $(shell ./bin/TaraDraw-config --osspec)

ifeq ($(OSSPEC), MacOS)
    TARA_SUBDIR := mac
else
    TARA_SUBDIR := x11
endif

all:
	${MAKE} -C $(TARA_SUBDIR)

clean:
	${MAKE} -C $(TARA_SUBDIR) clean

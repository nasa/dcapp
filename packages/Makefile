.NOTPARALLEL:
.PHONY: all clean

all: $(PACKAGES)
	${MAKE} -C basicutils
	${MAKE} -C TaraDraw
	${MAKE} -C PixelStream
	${MAKE} -C fontlib
	${MAKE} -C imgload

clean: $(PACKAGESCLEAN)
	${MAKE} -C basicutils clean
	${MAKE} -C TaraDraw clean
	${MAKE} -C PixelStream clean
	${MAKE} -C fontlib clean
	${MAKE} -C imgload clean

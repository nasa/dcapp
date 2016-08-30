.PHONY: all clean

all: $(PACKAGES)
	${MAKE} -C PixelStream
	${MAKE} -C TaraDraw
	${MAKE} -C fontlib
	${MAKE} -C imgload
	${MAKE} -C utils

clean: $(PACKAGESCLEAN)
	${MAKE} -C PixelStream clean
	${MAKE} -C TaraDraw clean
	${MAKE} -C fontlib clean
	${MAKE} -C imgload clean
	${MAKE} -C utils clean

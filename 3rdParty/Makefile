.PHONY: all clean

all: $(PACKAGES)
	${MAKE} -C can
	${MAKE} -C ccsds
	${MAKE} -C edge
	${MAKE} -C trick
	${MAKE} -C uei

clean: $(PACKAGESCLEAN)
	${MAKE} -C can clean
	${MAKE} -C ccsds clean
	${MAKE} -C edge clean
	${MAKE} -C trick clean
	${MAKE} -C uei clean

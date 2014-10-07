include ../../makedefs

HEADERS := tdDefines.hh tdAdapter.hh tdView.hh tdWindow.hh
OBJECTS := TaraDraw.o tdAdapter.o tdView.o tdWindow.o

BLD_OBJECTS := $(foreach obj, $(OBJECTS), $(OBJDIR)/$(obj))

$(LIBDIR)/libTD.a: $(BLD_OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $(BLD_OBJECTS)
	ranlib $@

$(OBJDIR)/%.o: %.mm $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)
	${LEGACY_CLEANUP}

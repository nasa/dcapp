include ../../makedefs

HEADERS := tdDefines.h tdAdapter.h tdView.h tdWindow.h
OBJECTS := TaraDraw.o tdAdapter.o tdView.o tdWindow.o

BLD_OBJECTS := $(foreach obj, $(OBJECTS), $(OBJDIR)/$(obj))

$(LIBDIR)/libTD.a: $(BLD_OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $(BLD_OBJECTS)
	ranlib $@

$(OBJDIR)/%.o: %.m $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)

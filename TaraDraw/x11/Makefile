include ../../makedefs

HEADERS := tdDefines.hh rotate.hh image.hh xfonts.hh
OBJECTS := TaraDraw.o rotate.o image.o

BLD_OBJECTS := $(foreach obj, $(OBJECTS), $(OBJDIR)/$(obj))

$(LIBDIR)/libTD.a: $(BLD_OBJECTS)
	mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $(BLD_OBJECTS)
	ranlib $@

$(OBJDIR)/%.o: %.cc $(HEADERS)
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) $(X11_CFLAG) -c $< -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)

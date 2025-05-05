
srcDir := $(CURDIR)
objDir := $(srcDir:$(DCAPP_HOME)%=$(BUILD_DIR)%)

srcFiles := $(wildcard *.frag *.vert *.tesc *.tese)
objFiles := $(srcFiles:%=$(objDir)/%.spv)

# ...

_ignore := $(info $(shell mkdir -p $(objDir)))

# ...

all: $(objFiles)

$(objDir)/%.spv: %
	glslangValidator -V $< -o $@

.PHONY: all

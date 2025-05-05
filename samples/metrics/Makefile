subMakes :=	logic

all: $(subMakes)

$(subMakes):
	$(MAKE) -C $@

clean:
	$(MAKE) clean -C $(subMakes)

.PHONY: all $(subMakes) clean

CC := glslangValidator

# stages named <name>.vert, <vert>.frag, etc. get
# converted to vert.spv, frag.spv, etc by glslang

SPV := vert.spv frag.spv
BUILD = $(CC) --target-env vulkan1.2 -t $^ -o $@

.PHONY: clean all

all: $(SPV)

vert.spv: shader.vert
	$(BUILD)

frag.spv: shader.frag
	$(BUILD)

clean:
	@rm -f *.spv


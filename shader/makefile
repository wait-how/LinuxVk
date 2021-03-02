CC := glslangValidator

# stages named <name>.vert, <vert>.frag, etc. get
# converted to vert.spv, frag.spv, etc by glslang

PIPELINE := shader
GLSLSRC := $(wildcard shader*)
SPV := vert.spv frag.spv

.PHONY: clean

all: $(SPV)

$(SPV): $(GLSLSRC)
	@$(CC) --target-env vulkan1.2 -t $^

clean:
	@rm -f *.spv


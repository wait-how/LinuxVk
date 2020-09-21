MAKEFLAGS += "-j $(shell nproc)"

CC := glslangValidator

GLSLSRC := shader.vert shader.frag

# stages named <name>.vert, <vert>.frag, etc. get
# converted to vert.spv, frag.spv, etc by glslang
SPV := vert.spv frag.spv

.PHONY: clean

$(SPV): $(GLSLSRC)
	$(CC) --target-env vulkan1.2 -t $^

clean:
	@rm -f *.spv


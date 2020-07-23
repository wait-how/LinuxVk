CC := glslangValidator

GLSLSRC := shader.vert shader.frag

.PHONY: all clean

all:
	@$(CC) -V shader.vert
	@$(CC) -V shader.frag

clean:
	@rm -f *.spv

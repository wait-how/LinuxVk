CC := glslangValidator

# stages named <name>.vert, <vert>.frag, etc. get
# converted to vert.spv, frag.spv, etc by glslang

# put compiled .spv files in .spv directory
SPVDIR := ../.spv

# grab anything that isn't a makefile
SHDRS := $(wildcard *.*)

# create .spv files in hidden directory
SPVS := $(addprefix $(SPVDIR)/,$(addsuffix .spv,$(SHDRS)))

BUILD = $(CC) --target-env vulkan1.2 -t $^ -o $@

# make .spv directory at startup
$(shell mkdir -p $(SPVDIR) > /dev/null)

# keep .spv files if make dies
.PRECIOUS: $(SPVDIR)/%.spv

.PHONY: clean all

# build all shader files by default
all: $(SPVS)

$(SPVDIR)/%.spv: %
	@$(BUILD)

clean:
	@rm -rf $(SPVDIR)

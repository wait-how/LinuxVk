# you can use the GCC -H option to list all included directories, helps with build debugging
# and make -d for makefile debugging

VPATH := ./src
CC := clang++

# get compilation options from libraries used, assuming they're actually installed
LIBS := $(shell pkg-config --libs glfw3 assimp glm vulkan)
LIBFLAGS := $(shell pkg-config --cflags glfw3 assimp glm vulkan)

DIRS := -Isrc
CFLAGS := -Wall -std=c++17 $(DIRS) $(LIBFLAGS)
LDFLAGS := 

# specify a single target name here
TARGETS := slow debug release small profile
MAINS := $(addsuffix .o, $(TARGETS) )

# specify a list of object files we want
OBJFILES := main.o camera.o

# all and clean aren't actually creating files, so they are declared as phony targets
.PHONY: top clean runprofile

# maximum information, for serious debugging
slow: CFLAGS += -ggdb -O0

# some optimization
debug: CFLAGS += -ggdb -Og

# fastest executable on my machine
release: CFLAGS += -Ofast -march=native -ffast-math -flto=thin
release: LDFLAGS += -flto=thin

# smaller executable
small: CFLAGS += -Os -s

# profiling run
profile: CFLAGS += -Ofast -march=native -fprofile-instr-generate -fcoverage-mapping
profile: LDFLAGS += -fprofile-instr-generate

# debug build by default
top: debug

# clean out .o and executable files
clean:
	@rm -f $(TARGETS) $(OBJFILES) default.prof*

spv:
	@make -C shader

# for each object file: that matches %.o: replace with %.cpp and use stb_image.h
$(OBJFILES): %.o: %.cpp %.h stb_image.h
	$(CC) -c -o $@ $< $(CFLAGS)

# for each target: compile in all dependancies as well as all library link flags we need
$(TARGETS): % : $(OBJFILES)
	$(CC) -o $@ $(LIBS) $^ $(LDFLAGS)

runprofile: profile
	@echo NOTE: executable has to exit for results to be generated.
	@./profile
	@llvm-profdata merge -sparse default.profraw -o default.profdata
	@llvm-cov show ./profile -instr-profile=default.profdata -show-line-counts-or-regions > default.proftxt
	@less default.proftxt

# for each cpp file: update timestamp, but don't create a file if you can't find it.
%.cpp:
	touch -c $@

%.h:
	touch -c $@


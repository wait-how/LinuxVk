# you can use the GCC -H option to list all included directories, helps with build debugging
# and make -d for makefile debugging

# makefile rules are specified here: https://www.gnu.org/software/make/manual/html_node/Rule-Syntax.html

# use all cores if needed
MAKEFLAGS += "-j $(shell nproc)" 

CXX := clang++
DB := lldb

# tell make to turn src/*.cpp into *.cpp
VPATH := src

LIBS := $(shell pkg-config --libs glfw3 assimp glm vulkan)
LIBFLAGS := $(shell pkg-config --cflags glfw3 assimp glm vulkan)

SRCFILES := $(foreach dir, $(VPATH), $(wildcard $(dir)/*.cpp))
CFLAGS := -Wall -std=c++17 $(DIRS) $(LIBFLAGS)

# map .cpp files to objects files of the same name
OBJFILES := $(SRCFILES:cpp=o)
LDFLAGS := 

# specify a single target name here
TARGETS := slow debug release small lineprofile profile
MAINS := $(addsuffix .o, $(TARGETS) )

# phony targets don't create a file as output
.PHONY: top clean runprofile

# maximum information, for serious debugging
slow: CFLAGS += -g$(DB) -O0

# some optimization
debug: CFLAGS += -g$(DB) -Og

# fastest executable on my machine
release: CFLAGS += -Ofast -march=native -mavx2 -ffast-math -flto=thin
release: LDFLAGS += -flto=thin

# smaller executable
small: CFLAGS += -Os -s

# line-based profiling run
lineprofile: CFLAGS += -Og -fprofile-instr-generate -fcoverage-mapping
lineprofile: LDFLAGS += -fprofile-instr-generate

# time-based profiling run
profile: CFLAGS += -pg
profile: LDFLAGS += -pg

# debug build by default
top: debug

# clean out .o and executable files
clean:
	@rm -f $(TARGETS) $(OBJFILES) default.prof* times.txt

spv:
	@make -C shader

# for each object file: that matches %.o: replace with %.cpp
$(OBJFILES): %.o: %.cpp
	$(CXX) -c -o $@ $< $(CFLAGS)

# for each target: compile in all dependancies as well as all library link flags we need
$(TARGETS): % : $(OBJFILES)
	$(CXX) -o $@ $(LIBS) $^ $(LDFLAGS)

# execute a profiling run and print out the results
getlineprofile: lineprofile
	@echo NOTE: executable has to exit for results to be generated.
	@./lineprofile
	@llvm-profdata merge -sparse default.profraw -o default.profdata
	@llvm-cov show ./lineprofile -instr-profile=default.profdata -show-line-counts-or-regions > default.proftxt
	@less default.proftxt

getprofile: profile
	@echo NOTE: executable has to exit for results to be generated.
	@./profile
	@gprof profile gmon.out -p > times.txt
	@less times.txt

# for each cpp file: update timestamp, but don't create a file if you can't find it.
%.cpp:
	touch -c $@

%.h:
	touch -c $@


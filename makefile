#_make_-d_prints_debug_info
#_makefile_rules_are_specified_here:_https://www.gnu.org/software/make/manual/html_node/Rule-Syntax.html

#_use_all_cores_if_possible
MAKEFLAGS_+=_-j_$(shell_nproc)

CXX_:=_clang++
DB_:=_lldb

#_directories_to_search_for_.cpp_or_.h_files
DIRS_:=_src

#_create_object_files_and_dependancy_files_in_hidden_dirs
OBJDIR_:=_.obj
DEPDIR_:=_.dep

LIB_CFLAGS_:=_$(shell_pkg-config_--cflags_glfw3_assimp_glm_vulkan)
LIB_LDFLAGS_:=_$(shell_pkg-config_--libs_glfw3_assimp_glm_vulkan)

#_generate_dependancy_information,_and_stick_it_in_depdir
DEPFLAGS_=_-MT_$@_-MMD_-MP_-MF_$(DEPDIR)/$*.Td

CFLAGS := -Wall -std=c++17 $(LIB_CFLAGS)
LDFLAGS := $(LIB_LDFLAGS)

SRCS_:=_$(wildcard_src/*.cpp)

#_if_any_word_(delimited_by_whitespace)_of_SRCS_(excluding_suffix)_matches_the_wildcard_'%',_put_it_in_the_object_or_dep_directory
OBJS_:=_$(patsubst_%,$(OBJDIR)/%.o,$(basename_$(SRCS)))
DEPS_:=_$(patsubst_%,$(DEPDIR)/%.d,$(basename_$(SRCS)))

#_make_hidden_subdirectories
$(shell_mkdir_-p_$(dir_$(OBJS))_>_/dev/null)
$(shell_mkdir_-p_$(dir_$(DEPS))_>_/dev/null)

.PHONY:_all_clean_spv_getlineprofile_gettimeprofile
BINS_:=_debug_bench_small_lineprofile_timeprofile

#_build_debug_by_default
all:_debug

#_important_warnings,_full_debug_info,_some_optimization
debug:_CFLAGS_+=_-Wextra_-g$(DB)_-Og

#_fastest_executable_on_my_machine
bench:_CFLAGS_+=_-Ofast_-march=native_-ffast-math_-flto=thin_-D_NDEBUG
bench:_LDFLAGS_+=_-flto=thin

#_smaller_executable
small:_CFLAGS_+=_-Os

lineprofile:_CFLAGS_+=_-Og_-fprofile-instr-generate_-fcoverage-mapping
lineprofile:_LDFLAGS_+=_-fprofile-instr-generate

timeprofile:_CFLAGS_+=_-pg
timeprofile:_LDFLAGS_+=_-pg

#_clean_out_.o_and_executable_files
clean:
	@rm_-f_$(BINS)
	@rm_-rf_.dep_.obj
	@rm_-f_default.prof*_times.txt_gmon.out

#_build_shaders
spv:
	@cd_shader_&&_$(MAKE)

#_execute_a_profiling_run_and_print_out_the_results
getlineprofile:_lineprofile
	@echo_NOTE:_executable_has_to_exit_for_results_to_be_generated.
	@./lineprofile
	@llvm-profdata_merge_-sparse_default.profraw_-o_default.profdata
	@llvm-cov_show_./lineprofile_-instr-profile=default.profdata_-show-line-counts-or-regions_>_default.proftxt
	@less_default.proftxt

gettimeprofile:_timeprofile
	@echo_NOTE:_executable_has_to_exit_for_results_to_be_generated.
	@./profile
	@gprof_profile_gmon.out_-p_>_times.txt
	@less_times.txt

#_link_executable_together_using_object_files_in_OBJDIR
$(BINS):_$(OBJS)
	@$(CXX)_-o_$@_$(LDFLAGS)_$^
	@echo_linked_$@

#_if_a_dep_file_is_available,_include_it_as_a_dependancy
#_when_including_the_dep_file,_don't_let_the_timestamp_of_the_file_determine_if_we_remake_the_target_since_the_dep
#_is_updated_after_the_target_is_built
$(OBJDIR)/%.o:_%.cpp
$(OBJDIR)/%.o:_%.cpp_|_$(DEPDIR)/%.d
	@$(CXX)_-c_-o_$@_$<_$(CFLAGS)_$(DEPFLAGS)
	@mv_-f_$(DEPDIR)/$*.Td_$(DEPDIR)/$*.d
	@echo_built_$(notdir_$@)

#_dep_files_are_not_deleted_if_make_dies
.PRECIOUS:_$(DEPDIR)/%.d

#_empty_dep_for_deps
$(DEPDIR)/%.d:_;

#_read_.d_files_if_nothing_else_matches,_ok_if_deps_don't_exist...?
-include_$(DEPS)

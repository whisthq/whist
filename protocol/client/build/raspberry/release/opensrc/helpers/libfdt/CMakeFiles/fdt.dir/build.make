# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.13

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/pi/Downloads/protocol/client

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/pi/Downloads/protocol/client/build/raspberry/release

# Include any dependencies generated for this target.
include opensrc/helpers/libfdt/CMakeFiles/fdt.dir/depend.make

# Include the progress variables for this target.
include opensrc/helpers/libfdt/CMakeFiles/fdt.dir/progress.make

# Include the compile flags for this target's objects.
include opensrc/helpers/libfdt/CMakeFiles/fdt.dir/flags.make

opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt.c.o: opensrc/helpers/libfdt/CMakeFiles/fdt.dir/flags.make
opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt.c.o: ../../../opensrc/helpers/libfdt/fdt.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pi/Downloads/protocol/client/build/raspberry/release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt.c.o"
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/fdt.dir/fdt.c.o   -c /home/pi/Downloads/protocol/client/opensrc/helpers/libfdt/fdt.c

opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/fdt.dir/fdt.c.i"
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pi/Downloads/protocol/client/opensrc/helpers/libfdt/fdt.c > CMakeFiles/fdt.dir/fdt.c.i

opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/fdt.dir/fdt.c.s"
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pi/Downloads/protocol/client/opensrc/helpers/libfdt/fdt.c -o CMakeFiles/fdt.dir/fdt.c.s

opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_empty_tree.c.o: opensrc/helpers/libfdt/CMakeFiles/fdt.dir/flags.make
opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_empty_tree.c.o: ../../../opensrc/helpers/libfdt/fdt_empty_tree.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pi/Downloads/protocol/client/build/raspberry/release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_empty_tree.c.o"
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/fdt.dir/fdt_empty_tree.c.o   -c /home/pi/Downloads/protocol/client/opensrc/helpers/libfdt/fdt_empty_tree.c

opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_empty_tree.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/fdt.dir/fdt_empty_tree.c.i"
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pi/Downloads/protocol/client/opensrc/helpers/libfdt/fdt_empty_tree.c > CMakeFiles/fdt.dir/fdt_empty_tree.c.i

opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_empty_tree.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/fdt.dir/fdt_empty_tree.c.s"
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pi/Downloads/protocol/client/opensrc/helpers/libfdt/fdt_empty_tree.c -o CMakeFiles/fdt.dir/fdt_empty_tree.c.s

opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_ro.c.o: opensrc/helpers/libfdt/CMakeFiles/fdt.dir/flags.make
opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_ro.c.o: ../../../opensrc/helpers/libfdt/fdt_ro.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pi/Downloads/protocol/client/build/raspberry/release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_ro.c.o"
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/fdt.dir/fdt_ro.c.o   -c /home/pi/Downloads/protocol/client/opensrc/helpers/libfdt/fdt_ro.c

opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_ro.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/fdt.dir/fdt_ro.c.i"
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pi/Downloads/protocol/client/opensrc/helpers/libfdt/fdt_ro.c > CMakeFiles/fdt.dir/fdt_ro.c.i

opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_ro.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/fdt.dir/fdt_ro.c.s"
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pi/Downloads/protocol/client/opensrc/helpers/libfdt/fdt_ro.c -o CMakeFiles/fdt.dir/fdt_ro.c.s

opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_rw.c.o: opensrc/helpers/libfdt/CMakeFiles/fdt.dir/flags.make
opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_rw.c.o: ../../../opensrc/helpers/libfdt/fdt_rw.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pi/Downloads/protocol/client/build/raspberry/release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_rw.c.o"
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/fdt.dir/fdt_rw.c.o   -c /home/pi/Downloads/protocol/client/opensrc/helpers/libfdt/fdt_rw.c

opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_rw.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/fdt.dir/fdt_rw.c.i"
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pi/Downloads/protocol/client/opensrc/helpers/libfdt/fdt_rw.c > CMakeFiles/fdt.dir/fdt_rw.c.i

opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_rw.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/fdt.dir/fdt_rw.c.s"
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pi/Downloads/protocol/client/opensrc/helpers/libfdt/fdt_rw.c -o CMakeFiles/fdt.dir/fdt_rw.c.s

opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_sw.c.o: opensrc/helpers/libfdt/CMakeFiles/fdt.dir/flags.make
opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_sw.c.o: ../../../opensrc/helpers/libfdt/fdt_sw.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pi/Downloads/protocol/client/build/raspberry/release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building C object opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_sw.c.o"
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/fdt.dir/fdt_sw.c.o   -c /home/pi/Downloads/protocol/client/opensrc/helpers/libfdt/fdt_sw.c

opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_sw.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/fdt.dir/fdt_sw.c.i"
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pi/Downloads/protocol/client/opensrc/helpers/libfdt/fdt_sw.c > CMakeFiles/fdt.dir/fdt_sw.c.i

opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_sw.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/fdt.dir/fdt_sw.c.s"
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pi/Downloads/protocol/client/opensrc/helpers/libfdt/fdt_sw.c -o CMakeFiles/fdt.dir/fdt_sw.c.s

opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_strerror.c.o: opensrc/helpers/libfdt/CMakeFiles/fdt.dir/flags.make
opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_strerror.c.o: ../../../opensrc/helpers/libfdt/fdt_strerror.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pi/Downloads/protocol/client/build/raspberry/release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_strerror.c.o"
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/fdt.dir/fdt_strerror.c.o   -c /home/pi/Downloads/protocol/client/opensrc/helpers/libfdt/fdt_strerror.c

opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_strerror.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/fdt.dir/fdt_strerror.c.i"
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pi/Downloads/protocol/client/opensrc/helpers/libfdt/fdt_strerror.c > CMakeFiles/fdt.dir/fdt_strerror.c.i

opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_strerror.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/fdt.dir/fdt_strerror.c.s"
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pi/Downloads/protocol/client/opensrc/helpers/libfdt/fdt_strerror.c -o CMakeFiles/fdt.dir/fdt_strerror.c.s

opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_wip.c.o: opensrc/helpers/libfdt/CMakeFiles/fdt.dir/flags.make
opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_wip.c.o: ../../../opensrc/helpers/libfdt/fdt_wip.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pi/Downloads/protocol/client/build/raspberry/release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building C object opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_wip.c.o"
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/fdt.dir/fdt_wip.c.o   -c /home/pi/Downloads/protocol/client/opensrc/helpers/libfdt/fdt_wip.c

opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_wip.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/fdt.dir/fdt_wip.c.i"
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pi/Downloads/protocol/client/opensrc/helpers/libfdt/fdt_wip.c > CMakeFiles/fdt.dir/fdt_wip.c.i

opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_wip.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/fdt.dir/fdt_wip.c.s"
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pi/Downloads/protocol/client/opensrc/helpers/libfdt/fdt_wip.c -o CMakeFiles/fdt.dir/fdt_wip.c.s

# Object files for target fdt
fdt_OBJECTS = \
"CMakeFiles/fdt.dir/fdt.c.o" \
"CMakeFiles/fdt.dir/fdt_empty_tree.c.o" \
"CMakeFiles/fdt.dir/fdt_ro.c.o" \
"CMakeFiles/fdt.dir/fdt_rw.c.o" \
"CMakeFiles/fdt.dir/fdt_sw.c.o" \
"CMakeFiles/fdt.dir/fdt_strerror.c.o" \
"CMakeFiles/fdt.dir/fdt_wip.c.o"

# External object files for target fdt
fdt_EXTERNAL_OBJECTS =

../../lib/libfdt.a: opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt.c.o
../../lib/libfdt.a: opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_empty_tree.c.o
../../lib/libfdt.a: opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_ro.c.o
../../lib/libfdt.a: opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_rw.c.o
../../lib/libfdt.a: opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_sw.c.o
../../lib/libfdt.a: opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_strerror.c.o
../../lib/libfdt.a: opensrc/helpers/libfdt/CMakeFiles/fdt.dir/fdt_wip.c.o
../../lib/libfdt.a: opensrc/helpers/libfdt/CMakeFiles/fdt.dir/build.make
../../lib/libfdt.a: opensrc/helpers/libfdt/CMakeFiles/fdt.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/pi/Downloads/protocol/client/build/raspberry/release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Linking C static library ../../../../../lib/libfdt.a"
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && $(CMAKE_COMMAND) -P CMakeFiles/fdt.dir/cmake_clean_target.cmake
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/fdt.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
opensrc/helpers/libfdt/CMakeFiles/fdt.dir/build: ../../lib/libfdt.a

.PHONY : opensrc/helpers/libfdt/CMakeFiles/fdt.dir/build

opensrc/helpers/libfdt/CMakeFiles/fdt.dir/clean:
	cd /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt && $(CMAKE_COMMAND) -P CMakeFiles/fdt.dir/cmake_clean.cmake
.PHONY : opensrc/helpers/libfdt/CMakeFiles/fdt.dir/clean

opensrc/helpers/libfdt/CMakeFiles/fdt.dir/depend:
	cd /home/pi/Downloads/protocol/client/build/raspberry/release && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/pi/Downloads/protocol/client /home/pi/Downloads/protocol/client/opensrc/helpers/libfdt /home/pi/Downloads/protocol/client/build/raspberry/release /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt /home/pi/Downloads/protocol/client/build/raspberry/release/opensrc/helpers/libfdt/CMakeFiles/fdt.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : opensrc/helpers/libfdt/CMakeFiles/fdt.dir/depend


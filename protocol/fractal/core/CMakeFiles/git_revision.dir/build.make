# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.21

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /opt/homebrew/Cellar/cmake/3.21.3/bin/cmake

# The command to remove a file.
RM = /opt/homebrew/Cellar/cmake/3.21.3/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/phil/Downloads/fractal/protocol

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/phil/Downloads/fractal/protocol

# Utility rule file for git_revision.

# Include any custom commands dependencies for this target.
include fractal/core/CMakeFiles/git_revision.dir/compiler_depend.make

# Include the progress variables for this target.
include fractal/core/CMakeFiles/git_revision.dir/progress.make

fractal/core/CMakeFiles/git_revision:
	cd /Users/phil/Downloads/fractal/protocol/fractal/core && /opt/homebrew/Cellar/cmake/3.21.3/bin/cmake -E echo_append "#define FRACTAL_GIT_REVISION " > /Users/phil/Downloads/fractal/protocol/include/fractal.sha.v
	cd /Users/phil/Downloads/fractal/protocol/fractal/core && git rev-parse --short HEAD >> /Users/phil/Downloads/fractal/protocol/include/fractal.sha.v
	cd /Users/phil/Downloads/fractal/protocol/fractal/core && /opt/homebrew/Cellar/cmake/3.21.3/bin/cmake -E copy_if_different /Users/phil/Downloads/fractal/protocol/include/fractal.sha.v /Users/phil/Downloads/fractal/protocol/include/fractal.v

git_revision: fractal/core/CMakeFiles/git_revision
git_revision: fractal/core/CMakeFiles/git_revision.dir/build.make
.PHONY : git_revision

# Rule to build all files generated by this target.
fractal/core/CMakeFiles/git_revision.dir/build: git_revision
.PHONY : fractal/core/CMakeFiles/git_revision.dir/build

fractal/core/CMakeFiles/git_revision.dir/clean:
	cd /Users/phil/Downloads/fractal/protocol/fractal/core && $(CMAKE_COMMAND) -P CMakeFiles/git_revision.dir/cmake_clean.cmake
.PHONY : fractal/core/CMakeFiles/git_revision.dir/clean

fractal/core/CMakeFiles/git_revision.dir/depend:
	cd /Users/phil/Downloads/fractal/protocol && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/phil/Downloads/fractal/protocol /Users/phil/Downloads/fractal/protocol/fractal/core /Users/phil/Downloads/fractal/protocol /Users/phil/Downloads/fractal/protocol/fractal/core /Users/phil/Downloads/fractal/protocol/fractal/core/CMakeFiles/git_revision.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : fractal/core/CMakeFiles/git_revision.dir/depend


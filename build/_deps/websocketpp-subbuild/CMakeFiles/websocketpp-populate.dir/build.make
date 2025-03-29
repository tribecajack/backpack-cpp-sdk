# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.31

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
CMAKE_COMMAND = /opt/homebrew/bin/cmake

# The command to remove a file.
RM = /opt/homebrew/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild

# Utility rule file for websocketpp-populate.

# Include any custom commands dependencies for this target.
include CMakeFiles/websocketpp-populate.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/websocketpp-populate.dir/progress.make

CMakeFiles/websocketpp-populate: CMakeFiles/websocketpp-populate-complete

CMakeFiles/websocketpp-populate-complete: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-install
CMakeFiles/websocketpp-populate-complete: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-mkdir
CMakeFiles/websocketpp-populate-complete: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-download
CMakeFiles/websocketpp-populate-complete: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-update
CMakeFiles/websocketpp-populate-complete: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-patch
CMakeFiles/websocketpp-populate-complete: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-configure
CMakeFiles/websocketpp-populate-complete: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-build
CMakeFiles/websocketpp-populate-complete: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-install
CMakeFiles/websocketpp-populate-complete: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-test
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Completed 'websocketpp-populate'"
	/opt/homebrew/bin/cmake -E make_directory /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild/CMakeFiles
	/opt/homebrew/bin/cmake -E touch /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild/CMakeFiles/websocketpp-populate-complete
	/opt/homebrew/bin/cmake -E touch /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild/websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-done

websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-update:
.PHONY : websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-update

websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-build: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-configure
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "No build step for 'websocketpp-populate'"
	cd /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-build && /opt/homebrew/bin/cmake -E echo_append
	cd /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-build && /opt/homebrew/bin/cmake -E touch /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild/websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-build

websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-configure: websocketpp-populate-prefix/tmp/websocketpp-populate-cfgcmd.txt
websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-configure: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-patch
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "No configure step for 'websocketpp-populate'"
	cd /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-build && /opt/homebrew/bin/cmake -E echo_append
	cd /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-build && /opt/homebrew/bin/cmake -E touch /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild/websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-configure

websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-download: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-gitinfo.txt
websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-download: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-mkdir
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Performing download step (git clone) for 'websocketpp-populate'"
	cd /Users/penrose/backpack-cpp-sdk/build/_deps && /opt/homebrew/bin/cmake -DCMAKE_MESSAGE_LOG_LEVEL=VERBOSE -P /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild/websocketpp-populate-prefix/tmp/websocketpp-populate-gitclone.cmake
	cd /Users/penrose/backpack-cpp-sdk/build/_deps && /opt/homebrew/bin/cmake -E touch /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild/websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-download

websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-install: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-build
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "No install step for 'websocketpp-populate'"
	cd /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-build && /opt/homebrew/bin/cmake -E echo_append
	cd /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-build && /opt/homebrew/bin/cmake -E touch /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild/websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-install

websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-mkdir:
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Creating directories for 'websocketpp-populate'"
	/opt/homebrew/bin/cmake -Dcfgdir= -P /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild/websocketpp-populate-prefix/tmp/websocketpp-populate-mkdirs.cmake
	/opt/homebrew/bin/cmake -E touch /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild/websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-mkdir

websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-patch: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-patch-info.txt
websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-patch: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-update
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "No patch step for 'websocketpp-populate'"
	/opt/homebrew/bin/cmake -E echo_append
	/opt/homebrew/bin/cmake -E touch /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild/websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-patch

websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-update:
.PHONY : websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-update

websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-test: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-install
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "No test step for 'websocketpp-populate'"
	cd /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-build && /opt/homebrew/bin/cmake -E echo_append
	cd /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-build && /opt/homebrew/bin/cmake -E touch /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild/websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-test

websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-update: websocketpp-populate-prefix/tmp/websocketpp-populate-gitupdate.cmake
websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-update: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-update-info.txt
websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-update: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-download
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Performing update step for 'websocketpp-populate'"
	cd /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-src && /opt/homebrew/bin/cmake -Dcan_fetch=YES -DCMAKE_MESSAGE_LOG_LEVEL=VERBOSE -P /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild/websocketpp-populate-prefix/tmp/websocketpp-populate-gitupdate.cmake

CMakeFiles/websocketpp-populate.dir/codegen:
.PHONY : CMakeFiles/websocketpp-populate.dir/codegen

websocketpp-populate: CMakeFiles/websocketpp-populate
websocketpp-populate: CMakeFiles/websocketpp-populate-complete
websocketpp-populate: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-build
websocketpp-populate: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-configure
websocketpp-populate: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-download
websocketpp-populate: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-install
websocketpp-populate: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-mkdir
websocketpp-populate: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-patch
websocketpp-populate: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-test
websocketpp-populate: websocketpp-populate-prefix/src/websocketpp-populate-stamp/websocketpp-populate-update
websocketpp-populate: CMakeFiles/websocketpp-populate.dir/build.make
.PHONY : websocketpp-populate

# Rule to build all files generated by this target.
CMakeFiles/websocketpp-populate.dir/build: websocketpp-populate
.PHONY : CMakeFiles/websocketpp-populate.dir/build

CMakeFiles/websocketpp-populate.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/websocketpp-populate.dir/cmake_clean.cmake
.PHONY : CMakeFiles/websocketpp-populate.dir/clean

CMakeFiles/websocketpp-populate.dir/depend:
	cd /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild /Users/penrose/backpack-cpp-sdk/build/_deps/websocketpp-subbuild/CMakeFiles/websocketpp-populate.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/websocketpp-populate.dir/depend


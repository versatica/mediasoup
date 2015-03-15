program_NAME := mediasoup


#
# Compiler
#
# CXX = g++-4.8
# CXX = g++-4.9 -fdiagnostics-color=auto
# CXX = clang++

#
# CXXFLAGS Options
# Comment/uncomment them during development.
#
DO_DEBUG_SYMBOLS := -g
# DO_SANITIZE := -fsanitize=thread -fPIE -pie
# DO_OPTIMIZE := -O3
# DO_PEDANTIC := -pedantic -Wno-gnu-zero-variadic-macro-arguments
#
CXXFLAGS += $(DO_DEBUG_SYMBOLS) $(DO_SANITIZE) $(DO_OPTIMIZE) $(DO_PEDANTIC)

#
# CXXFLAGS custom macros
# Added to the CXXFLAGS exported variable (before running make).
#
# -DMS_DEVEL enables MS_TRACE().


#
# Settings.
#

# Directory where the executable will be placed.
program_BIN_DIR := ./bin

# Directories containing program header files.
program_INCLUDE_DIRS := ./include

# External libraries with pkg-config support.
program_PKG_LIBRARIES = libuv >= 1.0.2, libconfig++ >= 1.4.8, openssl >= 1.0.1, libsrtp >= 1.5.1

# Other libraries to link with (don't put here those libs having pkg-config file).
program_OTHER_LIBRARIES := pthread

# Directories where search for other libraries.
# program_OTHER_LIBRARIES_DIRS := /usr/local/lib

# Directories containing other libraries header files.
# program_OTHER_LIBRARIES_INCLUDE_DIRS := /usr/local/include


# All the .cpp files.
program_CXX_SRCS := $(wildcard src/*.cpp src/*/*.cpp src/*/*/*.cpp src/*/*/*/*.cpp src/*/*/*/*/*.cpp)

# All the .h files.
program_CXX_INCLUDES := $(wildcard include/*.h include/*/*.h include/*/*/*.h include/*/*/*/*.h include/*/*/*/*/*.h)

# C++ object files to build.
program_CXX_OBJS := ${program_CXX_SRCS:.cpp=.o}

# Get all the .o files under src. Useful for 'make clean' as objects belinging to
# already deleted source files are also deleted.
program_all_CXX_OBJS = $(wildcard src/*.o src/*/*.o src/*/*/*.o src/*/*/*/*.o src/*/*/*/*/*.o)

# CPPFLAGS is the C preprocessor flags, so anything that compiles a C or C++ source
# file into an object file will use this flag.
# This adds -I$(includedir) for every include directory in $(program_INCLUDE_DIRS) and
# $(program_OTHER_LIBRARIES_INCLUDE_DIRS).
CPPFLAGS += $(foreach includedir, $(program_INCLUDE_DIRS), -I$(includedir))
CPPFLAGS += $(foreach includedir, $(program_OTHER_LIBRARIES_INCLUDE_DIRS), -I$(includedir))

# CXXFLAGS gives a list of flags that should be passed to the C++ compiler (use
# this, for example, to set the version of the C++ language, to specify the
# warning settings, or for other options specific to the C++ compiler).
CXXFLAGS += -std=c++11 -Wall

# LDFLAGS are used when linking, this will cause the appropriate flags to
# be passed to the linker.
# This adds -L$(librarydir) for every library directory given in $(program_OTHER_LIBRARIES_DIRS)
# and -l$(library) for every library given in $(program_OTHER_LIBRARIES).
LDFLAGS += $(foreach librarydir, $(program_OTHER_LIBRARIES_DIRS), -L$(librarydir))
LDFLAGS += $(foreach library, $(program_OTHER_LIBRARIES), -l$(library))

# Add flags required by external libraries with pkg-config support.
CPPFLAGS += $(shell pkg-config --cflags "$(program_PKG_LIBRARIES)")
LDFLAGS += $(shell pkg-config --libs "$(program_PKG_LIBRARIES)")


#
# Targets
#

# This indicates that "all", program_NAME, "check_pkg_libraries" and "clean" are
# "phony targets". Therefore calling "make XXXX" should execute the content of its
# build rules, even if a newer file named "XXXX" exists.
.PHONY: all $(program_NAME) check_pkg_libraries clean distclean

# This is first build rule in the Makefile, and so executing "make" and executing
# "make all" are the same. The target simply depends on $(program_NAME).
all: $(program_NAME)

# The program depends on the object files (which are automatically built using the
# predefined build rules, nothing needs to be given explicitly for them).
# The build rule $(LINK.cc) is used to link the object files and output a file with
# the same name as the program. Note that LINK.cc makes use of CXX, CXXFLAGS,
# LFLAGS, etc. LINK.cc is usually defined as: $(CXX) $(CXXFLAGS) $(CPPFLAGS)
# $(LDFLAGS) $(TARGET_ARCH).
$(program_NAME): check_pkg_libraries $(program_CXX_OBJS)
	mkdir -p $(program_BIN_DIR)
	$(LINK.cc) $(program_CXX_OBJS) -o $(program_BIN_DIR)/$(program_NAME)

# Check the required libraries with pkg-support.
check_pkg_libraries:
	pkg-config --exists --print-errors "$(program_PKG_LIBRARIES)"

# This target removes the built program and the generated object files. The @ symbol
# indicates that the line should be run silently, and the - symbol indicates that
# errors should be ignored (i.e., the file already doesn't exist).
clean:
	@- $(RM) $(program_NAME)
	@- $(RM) $(program_all_CXX_OBJS)

# doc: check_pkg_libraries $(program_CXX_OBJS)
	# cldoc generate $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) --output cldoc $(program_CXX_SRCS)

doc: check_pkg_libraries $(program_CXX_OBJS)
	cldoc generate $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) -- --output cldoc $(program_CXX_SRCS) $(program_CXX_INCLUDES)

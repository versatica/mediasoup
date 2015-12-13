APP_NAME := mediasoup-worker

#
# Compiler.
#
# CXX := g++-4.8
# CXX := g++-4.9 -fdiagnostics-color=auto
# CXX := clang++

#
# Development flags.
# Comment/uncomment them during development.
#
DO_DEBUG_SYMBOLS := -g
# DO_SANITIZE := -fsanitize=thread -fPIE -pie
# DO_OPTIMIZE := -O3
# DO_PEDANTIC := -pedantic -Wno-gnu-zero-variadic-macro-arguments
#
DEV_FLAGS := $(DO_DEBUG_SYMBOLS) $(DO_SANITIZE) $(DO_OPTIMIZE) $(DO_PEDANTIC)

#
# Custom macros.
# Added to the CXXFLAGS exported variable (before running make).
#
# -DMS_DEVEL enables MS_TRACE().

#
# Settings.
#

# App source files.
APP_SOURCES := $(shell find ./src -name "*.cpp")

# App headers directory.
APP_HEADERS_DIR := ./include

# App header files (just for documentation).
APP_HEADERS := $(shell find $(APP_HEADERS_DIR) -name "*.h")

# App binary directory.
APP_BIN_DIR := ./bin

# System libraries to link with.
SYSTEM_LIBS := pthread

# External libraries with pkg-config support.
PKG_LIBS :=
PKG_LIBS += libuv >= 1.6.1
PKG_LIBS += openssl >= 1.0.1
PKG_LIBS += libsrtp >= 1.5.2

# Dependencies source files.
DEP_SOURCES :=
DEP_SOURCES += ./submodules/jsoncpp/dist/jsoncpp.cpp

# Dependencies header directories.
DEP_HEADERS_DIRS :=
DEP_HEADERS_DIRS += ./submodules/jsoncpp/dist
DEP_HEADERS_DIRS += ./submodules/usrsctp/usrsctplib
DEP_HEADERS_DIRS += ./submodules/picojson

# Dependencies static libraries to link with.
DEP_LIBS :=
DEP_LIBS += ./submodules/usrsctp/usrsctplib/.libs/libusrsctp.a

# Object files to build.
ALL_OBJS := ${APP_SOURCES:.cpp=.o} ${DEP_SOURCES:.cpp=.o}

# CPPFLAGS is the C preprocessor flags, so anything that compiles a C or C++ source
# file into an object file will use this flag.
CPPFLAGS += -I$(APP_HEADERS_DIR)
CPPFLAGS += $(foreach dir, $(DEP_HEADERS_DIRS), -I$(dir))
CPPFLAGS += $(shell pkg-config --cflags "$(PKG_LIBS)")

# CXXFLAGS gives a list of flags that should be passed to the C++ compiler (use
# this, for example, to set the version of the C++ language, to specify the
# warning settings, or for other options specific to the C++ compiler).
CXXFLAGS += -std=c++11 -Wall $(DEV_FLAGS)

# LDFLAGS are used when linking, this will cause the appropriate flags to
# be passed to the linker.
LDFLAGS += $(foreach lib, $(SYSTEM_LIBS), -l$(lib))
LDFLAGS += $(shell pkg-config --libs "$(PKG_LIBS)")

#
# Targets.
#

# This indicates that those names are "phony targets". Therefore calling
# "make XXXX" should execute the content of its build rules, even if a newer
# file named "XXXX" exists.
.PHONY: default $(APP_NAME) check_pkg_libs echo_compiling_all_objs clean submodules doc

# This is first build rule in the Makefile, and so executing "make" and executing
# "make default" are the same. The target simply depends on $(APP_NAME).
default: $(APP_NAME)

# The app depends on the object files (which are automatically built using the
# predefined build rules, nothing needs to be given explicitly for them).
# The build rule $(LINK.cc) is used to link the object files and output a file with
# the same name as the app. Note that LINK.cc makes use of CXX, CXXFLAGS,
# LFLAGS, etc. LINK.cc is usually defined as: $(CXX) $(CXXFLAGS) $(CPPFLAGS)
# $(LDFLAGS) $(TARGET_ARCH).
$(APP_NAME): check_pkg_libs echo_compiling_all_objs $(ALL_OBJS)
	@ mkdir -p $(APP_BIN_DIR)
	@ echo "INFO: linking .o objects and libraries into $(APP_BIN_DIR)/$(APP_NAME) ..."
	$(LINK.cc) $(ALL_OBJS) $(DEP_LIBS) -o $(APP_BIN_DIR)/$(APP_NAME)
	@ echo "INFO: executable generated at $(APP_BIN_DIR)/$(APP_NAME)"

# Check required libraries with pkg-support.
check_pkg_libs:
	@ echo "INFO: checking required PKG libraries ($(strip $(PKG_LIBS))) ..."
	@ pkg-config --exists --print-errors "$(PKG_LIBS)"

echo_compiling_all_objs:
	@ echo "INFO: compiling source files into .o object files ..."

# This target removes the built app and the generated object files. The @ symbol
# indicates that the line should be run silently, and the - symbol indicates that
# errors should be ignored (i.e., the file doesn't exist).
clean:
	@ echo "INFO: deleting $(APP_BIN_DIR) folder ..."
	@ $(RM) -rf $(APP_BIN_DIR)
	@ echo "INFO: deleting .o object files ..."
	@ $(RM) $(ALL_OBJS)

submodules:
	@ echo "INFO: retrieving Git submodules ..."
	git submodule init
	git submodule update
	@ echo "INFO: building jsoncpp lib ..."
	cd ./submodules/jsoncpp && python amalgamate.py >/dev/null
	@ echo "INFO: building usrsctp lib ..."
	cd ./submodules/usrsctp && ./bootstrap && autoconf && ./configure && make

doc:
	@ echo "INFO: generating documentation ..."
	cldoc generate $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) -- --output cldoc $(APP_SOURCES) $(APP_HEADERS)

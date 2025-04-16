# This place is not a place of honor.
# No highly esteemed deed is commemorated here.
# Nothing valued is here.

# This is kept here only in case of emergencies.
# This is a brittle, fundamentally incorrect way of compiling these dependencies.

# If RACK_DIR is not defined when calling the Makefile, default to two directories above
RACK_DIR ?= ../..

# FLAGS will be passed to both the C and C++ compiler
FLAGS += -DFMT_ENFORCE_COMPILE_STRING=1 -Ilibs/fmt/include -Ilibs/vcv-rackthemer/include -Ilibs/header_only_libs
CFLAGS +=
CXXFLAGS += -std=c++17

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.
LDFLAGS +=

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp)
SOURCES += $(wildcard src/**/*.cpp)
SOURCES += $(wildcard src/**/**/*.cpp)
SOURCES += $(wildcard src/**/**/**/*.cpp)
SOURCES += $(wildcard libs/fmt/src/format.cc)
SOURCES += $(wildcard libs/vcv-rackthemer/src/*.cpp)

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)
DISTRIBUTABLES += LICENSE.md
DISTRIBUTABLES += license
DISTRIBUTABLES += README.md


# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk

CXXFLAGS := $(filter-out -std=c++11,$(CXXFLAGS))
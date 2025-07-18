RACK_DIR ?= ../..
include $(RACK_DIR)/arch.mk

EXTRA_CMAKE :=
RACK_PLUGIN_NAME := plugin
RACK_PLUGIN_EXT := so

ifdef ARCH_WIN
	RACK_PLUGIN_EXT := dll
endif

ifdef ARCH_MAC
	EXTRA_CMAKE := -DCMAKE_OSX_ARCHITECTURES="x86_64"
	RACK_PLUGIN_EXT := dylib
	ifdef ARCH_ARM64
		EXTRA_CMAKE := -DCMAKE_OSX_ARCHITECTURES="arm64"
	endif
endif

RACK_PLUGIN := $(RACK_PLUGIN_NAME).$(RACK_PLUGIN_EXT)

CMAKE_BUILD ?= build/cmake-build
cmake_rack_plugin := $(CMAKE_BUILD)/$(RACK_PLUGIN)

$(info cmake_rack_plugin target is '$(cmake_rack_plugin)')

$(cmake_rack_plugin): CMakeLists.txt .FORCE
	$(CMAKE) -B $(CMAKE_BUILD) -DRACK_SDK_DIR=$(RACK_DIR) -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(CMAKE_BUILD)/dist $(EXTRA_CMAKE)
	cmake --build $(CMAKE_BUILD) -- -j $(shell getconf _NPROCESSORS_ONLN)
	cmake --install $(CMAKE_BUILD)

# Add files to the ZIP package when running `make dist`
dist: res

DISTRIBUTABLES += res LICENSE.md license README.md

# Include the VCV plugin Makefile framework
include $(RACK_DIR)/plugin.mk

all: $(cmake_rack_plugin) ;

.PHONY: .FORCE
.FORCE:
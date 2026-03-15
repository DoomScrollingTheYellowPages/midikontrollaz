# =============================================================================
# M-Vave SMC Pad — VCV Rack 2 Plugin
# Makefile: thin CMake wrapper
#
# USAGE:
#   make           # configure + build (downloads Rack SDK automatically)
#   make install   # install to your Rack plugins folder
#   make dist      # create distributable .zip package in dist/
#   make clean     # remove build/ and dist/
# =============================================================================

BUILD_DIR ?= build

SLUG    := $(shell jq -r '.slug'    plugin.json 2>/dev/null || echo MVaveSmcPad)
VERSION := $(shell jq -r '.version' plugin.json 2>/dev/null || echo 2.0.0)

UNAME := $(shell uname -s 2>/dev/null)
ifeq ($(UNAME),Darwin)
  PLUGIN_FILE   := plugin.dylib
  ARCH          := $(shell uname -m)
  ifeq ($(ARCH),arm64)
    DIST_PLATFORM := mac-arm64
  else
    DIST_PLATFORM := mac-x64
  endif
else ifeq ($(UNAME),Linux)
  PLUGIN_FILE   := plugin.so
  DIST_PLATFORM := lin-x64
else
  PLUGIN_FILE   := plugin.dll
  DIST_PLATFORM := win-x64
endif

ifdef RACK_DIR
  CMAKE_RACK_DIR := -DRACK_DIR=$(RACK_DIR)
else
  CMAKE_RACK_DIR :=
endif

.PHONY: all install dist clean

$(BUILD_DIR)/CMakeCache.txt: CMakeLists.txt plugin.json
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release $(CMAKE_RACK_DIR)

$(BUILD_DIR)/$(PLUGIN_FILE): $(BUILD_DIR)/CMakeCache.txt
	cmake --build $(BUILD_DIR) --parallel

all: $(BUILD_DIR)/$(PLUGIN_FILE)

install: $(BUILD_DIR)/$(PLUGIN_FILE)
	cmake --install $(BUILD_DIR)

dist: $(BUILD_DIR)/$(PLUGIN_FILE)
	cmake --build $(BUILD_DIR) --target dist
	@echo "Created dist/$(SLUG)-$(VERSION)-$(DIST_PLATFORM).zip"

clean:
	rm -rf $(BUILD_DIR) dist

cmake_minimum_required(VERSION 3.21)
project(MVaveSmcPad VERSION 2.0.0 LANGUAGES CXX)
# NOTE: The major version MUST be 2 — VCV Rack 2 rejects plugins whose
# version does not start with "2." (ABI compatibility check).

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Detect platform/architecture ──────────────────────────────────────────────
if(APPLE)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    set(RACK_ARCH "mac-arm64")
  else()
    set(RACK_ARCH "mac-x64")
  endif()
  set(PLUGIN_EXT ".dylib")
elseif(WIN32)
  set(RACK_ARCH "win-x64")
  set(PLUGIN_EXT ".dll")
else()
  set(RACK_ARCH "lin-x64")
  set(PLUGIN_EXT ".so")
endif()

# ── Rack SDK auto-download ────────────────────────────────────────────────────
set(RACK_SDK_VERSION "2.6.6")

if(NOT DEFINED RACK_DIR)
  if(DEFINED ENV{RACK_DIR})
    set(RACK_DIR $ENV{RACK_DIR})
  else()
    set(RACK_SDK_URL "https://vcvrack.com/downloads/Rack-SDK-${RACK_SDK_VERSION}-${RACK_ARCH}.zip")
    set(RACK_SDK_DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/_deps/rack-sdk")
    set(RACK_DIR "${RACK_SDK_DOWNLOAD_DIR}/Rack-SDK")

    if(NOT EXISTS "${RACK_DIR}/include/rack.hpp")
      message(STATUS "Downloading Rack SDK from ${RACK_SDK_URL}...")
      file(MAKE_DIRECTORY "${RACK_SDK_DOWNLOAD_DIR}")
      set(RACK_SDK_ZIP "${RACK_SDK_DOWNLOAD_DIR}/Rack-SDK.zip")
      file(DOWNLOAD "${RACK_SDK_URL}" "${RACK_SDK_ZIP}"
        STATUS RACK_SDK_DOWNLOAD_STATUS SHOW_PROGRESS TLS_VERIFY ON)
      list(GET RACK_SDK_DOWNLOAD_STATUS 0 RACK_SDK_DOWNLOAD_CODE)
      list(GET RACK_SDK_DOWNLOAD_STATUS 1 RACK_SDK_DOWNLOAD_MESSAGE)
      if(NOT RACK_SDK_DOWNLOAD_CODE EQUAL 0)
        message(FATAL_ERROR "Failed to download Rack SDK: ${RACK_SDK_DOWNLOAD_MESSAGE}")
      endif()
      message(STATUS "Extracting Rack SDK...")
      file(ARCHIVE_EXTRACT INPUT "${RACK_SDK_ZIP}" DESTINATION "${RACK_SDK_DOWNLOAD_DIR}")
      if(NOT EXISTS "${RACK_DIR}/include/rack.hpp")
        message(FATAL_ERROR "Failed to extract Rack SDK.")
      endif()
      message(STATUS "Rack SDK downloaded successfully")
    else()
      message(STATUS "Using cached Rack SDK at: ${RACK_DIR}")
    endif()
  endif()
endif()

message(STATUS "Using Rack SDK at: ${RACK_DIR}")

# ── Generate plugin.json from template ────────────────────────────────────────
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/plugin.json.in
  ${CMAKE_CURRENT_BINARY_DIR}/plugin.json
  @ONLY
)

# ── Rack SDK paths ────────────────────────────────────────────────────────────
set(RACK_INCLUDE_DIR ${RACK_DIR}/include)
set(RACK_DEP_DIR     ${RACK_DIR}/dep/include)
if(WIN32)
  set(RACK_LIB ${RACK_DIR}/libRack.dll.a)
else()
  set(RACK_LIB ${RACK_DIR}/libRack${CMAKE_SHARED_LIBRARY_SUFFIX})
endif()

if(NOT EXISTS ${RACK_INCLUDE_DIR}/rack.hpp)
  message(FATAL_ERROR "rack.hpp not found in ${RACK_INCLUDE_DIR}. Check RACK_DIR.")
endif()

# ── Plugin shared library ─────────────────────────────────────────────────────
add_library(MVaveSmcPad MODULE
  src/plugin.cpp
  src/SmcPad.cpp
)

set_target_properties(MVaveSmcPad PROPERTIES
  PREFIX      ""
  SUFFIX      ${PLUGIN_EXT}
  OUTPUT_NAME "plugin"
)

target_include_directories(MVaveSmcPad PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${RACK_INCLUDE_DIR}
  ${RACK_DEP_DIR}
)

target_compile_options(MVaveSmcPad PRIVATE
  -fPIC -O3 -funsafe-math-optimizations -fno-omit-frame-pointer
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wno-unused-parameter>
)

if(RACK_ARCH STREQUAL "lin-x64" OR RACK_ARCH STREQUAL "win-x64")
  target_compile_options(MVaveSmcPad PRIVATE -march=nehalem)
elseif(RACK_ARCH STREQUAL "mac-x64")
  target_compile_options(MVaveSmcPad PRIVATE -march=nehalem)
endif()

string(REPLACE "-" "_" RACK_ARCH_MACRO "${RACK_ARCH}")
target_compile_definitions(MVaveSmcPad PRIVATE
  ARCH_${RACK_ARCH_MACRO}
  $<$<PLATFORM_ID:Windows>:_USE_MATH_DEFINES>
)

if(APPLE)
  target_link_options(MVaveSmcPad PRIVATE
    -undefined dynamic_lookup
    -Wl,-install_name,@rpath/plugin.dylib
  )
elseif(UNIX)
  target_link_options(MVaveSmcPad PRIVATE -Wl,--allow-shlib-undefined)
  target_link_libraries(MVaveSmcPad PRIVATE pthread)
elseif(WIN32)
  target_link_libraries(MVaveSmcPad PRIVATE ${RACK_LIB} ws2_32)
endif()

# ── Install target ────────────────────────────────────────────────────────────
if(APPLE)
  set(RACK_PLUGINS_DIR "$ENV{HOME}/Library/Application Support/Rack2/plugins-${RACK_ARCH}")
elseif(WIN32)
  set(RACK_PLUGINS_DIR "$ENV{LOCALAPPDATA}/Rack2/plugins-${RACK_ARCH}")
else()
  set(RACK_PLUGINS_DIR "$ENV{HOME}/.Rack2/plugins")
endif()

set(PLUGIN_INSTALL_DIR ${RACK_PLUGINS_DIR}/MVaveSmcPad)

install(TARGETS MVaveSmcPad LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugin.json DESTINATION ${PLUGIN_INSTALL_DIR})
install(DIRECTORY res/ DESTINATION ${PLUGIN_INSTALL_DIR}/res OPTIONAL)

message(STATUS "Install target: ${PLUGIN_INSTALL_DIR}")

# ── Package target ────────────────────────────────────────────────────────────
set(PACKAGE_ROOT_DIR "${CMAKE_BINARY_DIR}/package/${PROJECT_NAME}")
set(DIST_DIR "${CMAKE_SOURCE_DIR}/dist")
set(DIST_ARCHIVE_PATH "${DIST_DIR}/${PROJECT_NAME}-${PROJECT_VERSION}-${RACK_ARCH}.zip")

add_custom_target(dist
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/package"
  COMMAND ${CMAKE_COMMAND} -E rm -rf "${PACKAGE_ROOT_DIR}"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_ROOT_DIR}"
  COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:MVaveSmcPad>" "${PACKAGE_ROOT_DIR}/plugin${PLUGIN_EXT}"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/plugin.json" "${PACKAGE_ROOT_DIR}/plugin.json"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/res" "${PACKAGE_ROOT_DIR}/res"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DIR}"
  COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_BINARY_DIR}/package" ${CMAKE_COMMAND} -E tar "cf" "${DIST_ARCHIVE_PATH}" --format=zip "${PROJECT_NAME}"
  DEPENDS MVaveSmcPad
  COMMENT "Packaging ${DIST_ARCHIVE_PATH}"
)

# The Flutter tooling requires that developers have CMake 3.10 or later
# installed. You should not increase this version, as doing so will cause
# the plugin to fail to compile for some customers of the plugin.
cmake_minimum_required(VERSION 3.10)

project(jump_library VERSION 0.0.1 LANGUAGES C)

add_library(jump SHARED
  "jump.cpp"
  "altcalc.cpp"
)

set_target_properties(jump PROPERTIES
  PUBLIC_HEADER jump.h
  OUTPUT_NAME "jump"
)

target_compile_definitions(jump PUBLIC DART_SHARED_LIB)

if (ANDROID)
  # Support Android 15 16k page size
  target_link_options(jump PRIVATE "-Wl,-z,max-page-size=16384")
endif()

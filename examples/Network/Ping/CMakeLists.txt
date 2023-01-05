cmake_minimum_required(VERSION 3.0)

project(ExamplePing)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

add_executable(ExamplePing main.cpp)

set_target_properties(ExamplePing PROPERTIES LINK_FLAGS "${SLIB_LINK_STATIC_FLAGS}")

target_link_libraries (
  ExamplePing
  slib
  pthread
  dl
)

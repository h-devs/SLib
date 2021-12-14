cmake_minimum_required(VERSION 3.0)

project(ExampleCamera)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

add_executable(ExampleCamera
  main.cpp
  ../src/app.cpp
)

set_target_properties(ExampleCamera PROPERTIES LINK_FLAGS "${SLIB_LINK_STATIC_FLAGS}")

target_link_libraries (
  ExampleCamera
  slib
  pthread
  dl
)

cmake_minimum_required(VERSION 3.0)

project(ExampleCamera)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

add_executable(ExampleCamera
  main.cpp
  ../src/app.cpp
)

set_target_properties(ExampleCamera PROPERTIES LINK_FLAGS "-static-libgcc -static-libstdc++ -Wl,--wrap=memcpy")

target_link_libraries (
  ExampleCamera
  slib
  pthread
  dl
)

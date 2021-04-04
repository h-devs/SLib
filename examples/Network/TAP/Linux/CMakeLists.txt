cmake_minimum_required(VERSION 3.0)

project(Tap)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

add_executable(Tap
  ../main.cpp
)

set_target_properties(Tap PROPERTIES LINK_FLAGS "-static-libgcc -static-libstdc++ -Wl,--wrap=memcpy")

target_link_libraries (
  Tap
  slib
  pthread
  dl
)

cmake_minimum_required(VERSION 3.0)

project(IPC)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

add_executable(IPC
  main.cpp
)

set_target_properties(IPC PROPERTIES LINK_FLAGS "-static-libgcc -Wl,--wrap=memcpy")

target_link_libraries (
  IPC
  slib
  pthread
  dl
)

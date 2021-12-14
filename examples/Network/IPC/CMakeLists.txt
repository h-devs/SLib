cmake_minimum_required(VERSION 3.0)

project(IPC)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

add_executable(IPC
  main.cpp
)

set_target_properties(IPC PROPERTIES LINK_FLAGS "${SLIB_LINK_STATIC_FLAGS}")

target_link_libraries (
  IPC
  slib
  pthread
  dl
)

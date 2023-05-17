cmake_minimum_required(VERSION 3.0)

project(SocketEvent)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

add_executable(SocketEvent
  main.cpp
)

set_target_properties(SocketEvent PROPERTIES LINK_FLAGS "${SLIB_LINK_STATIC_FLAGS}")

target_link_libraries (
  SocketEvent
  slib
  pthread
  dl
)

cmake_minimum_required(VERSION 3.0)

project(TestSocketEvent)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

add_executable(TestSocketEvent
  main.cpp
)

set_target_properties(TestSocketEvent PROPERTIES LINK_FLAGS "${SLIB_LINK_STATIC_FLAGS}")

target_link_libraries (
  TestSocketEvent
  slib
  pthread
  dl
)

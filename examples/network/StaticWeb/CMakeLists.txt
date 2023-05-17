cmake_minimum_required(VERSION 3.0)

project(StaticWeb)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

add_executable(StaticWeb
  main.cpp
)

set_target_properties(StaticWeb PROPERTIES LINK_FLAGS "${SLIB_LINK_STATIC_FLAGS}")

target_link_libraries (
  StaticWeb
  slib
  pthread
  dl
)

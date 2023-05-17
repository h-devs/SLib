cmake_minimum_required(VERSION 3.0)

project(Mirror)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

add_executable(Mirror
  ../src/main.cpp
)

set_target_properties(Mirror PROPERTIES LINK_FLAGS "${SLIB_LINK_STATIC_FLAGS}")

target_link_libraries (
  Mirror
  slib
  pthread
  dl
)

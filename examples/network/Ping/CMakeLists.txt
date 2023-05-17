cmake_minimum_required(VERSION 3.0)

project(Ping)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

add_executable(Ping main.cpp)

set_target_properties(Ping PROPERTIES LINK_FLAGS "${SLIB_LINK_STATIC_FLAGS}")

target_link_libraries (
  Ping
  slib
  pthread
  dl
)

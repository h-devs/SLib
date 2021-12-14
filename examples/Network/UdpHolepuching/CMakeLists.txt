cmake_minimum_required(VERSION 3.0)

project(ExampleUdpHolepunching)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

add_executable(ExampleUdpHolepunching main.cpp)

set_target_properties(ExampleUdpHolepunching PROPERTIES LINK_FLAGS "${SLIB_LINK_STATIC_FLAGS}")

target_link_libraries (
  ExampleUdpHolepunching
  slib
  pthread
)

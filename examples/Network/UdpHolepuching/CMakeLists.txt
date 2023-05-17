cmake_minimum_required(VERSION 3.0)

project(UdpHolepunching)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

add_executable(UdpHolepunching main.cpp)

set_target_properties(UdpHolepunching PROPERTIES LINK_FLAGS "${SLIB_LINK_STATIC_FLAGS}")

target_link_libraries (
  UdpHolepunching
  slib
  pthread
  dl
)

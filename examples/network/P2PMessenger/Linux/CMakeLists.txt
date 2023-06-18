cmake_minimum_required(VERSION 3.0)

project(P2PMsg)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

add_executable(P2PMsg
  main.cpp
  ../app.cpp
)

set_target_properties(P2PMsg PROPERTIES LINK_FLAGS "${SLIB_LINK_STATIC_FLAGS}")

target_link_libraries (
  P2PMsg
  slib
  pthread
  dl
)

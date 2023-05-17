cmake_minimum_required(VERSION 3.0)

project(GetMachineSN)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

add_executable(GetMachineSN
  ../src/main.cpp
)

set_target_properties(GetMachineSN PROPERTIES LINK_FLAGS "${SLIB_LINK_STATIC_FLAGS}")

target_link_libraries (
  GetMachineSN
  slib
  pthread
  dl
)

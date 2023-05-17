cmake_minimum_required(VERSION 2.6)

project(ARC)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

add_executable(ARC main.cpp)
target_link_libraries (
  ARC
  slib
  pthread
  dl
)

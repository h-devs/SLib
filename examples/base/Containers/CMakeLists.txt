cmake_minimum_required(VERSION 3.0)

project(Containers)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

add_executable(Containers main.cpp)
target_link_libraries (
  Containers
  slib
  pthread
  dl
)

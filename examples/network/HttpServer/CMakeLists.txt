cmake_minimum_required(VERSION 3.0)

project(HttpServer)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

add_executable(HttpServer main.cpp)
target_link_libraries (
  HttpServer
  slib
  pthread
  dl
)

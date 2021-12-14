cmake_minimum_required(VERSION 3.0)

project(ExampleImageView)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

add_executable(ExampleImageView
  ../src/app.cpp
  main.cpp
)

set_target_properties(ExampleImageView PROPERTIES LINK_FLAGS "${SLIB_LINK_STATIC_FLAGS}")

target_link_libraries (
  ExampleImageView
  slib
  pthread
  dl
)

file(COPY ../images DESTINATION .)

cmake_minimum_required(VERSION 3.0)

project(ExampleImageView)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

add_executable(ExampleImageView
  ../src/app.cpp
  main.cpp
)

set_target_properties(ExampleImageView PROPERTIES LINK_FLAGS "-static-libgcc -Wl,--wrap=memcpy")

target_link_libraries (
  ExampleImageView
  slib
  pthread
  dl
  gobject-2.0
  glib-2.0
)

file(COPY ../images DESTINATION .)

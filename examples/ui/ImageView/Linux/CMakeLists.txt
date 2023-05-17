cmake_minimum_required(VERSION 3.0)

project(ImageView)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

add_executable(ImageView
  ../src/app.cpp
  main.cpp
)

set_target_properties(ImageView PROPERTIES LINK_FLAGS "${SLIB_LINK_STATIC_FLAGS}")

target_link_libraries (
  ImageView
  slib
  pthread
  dl
)

file(COPY ../images DESTINATION .)

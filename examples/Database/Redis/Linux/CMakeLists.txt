cmake_minimum_required(VERSION 3.0)

project(RedisClientExample)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

link_directories("${SLIB_PATH}/external/lib/Linux/${CMAKE_SYSTEM_PROCESSOR}")

add_executable(RedisClientExample
  ../main.cpp
)

set_target_properties(RedisClientExample PROPERTIES LINK_FLAGS "${SLIB_LINK_STATIC_FLAGS}")

target_link_libraries (
  RedisClientExample
  slib
  hiredis
  pthread
)

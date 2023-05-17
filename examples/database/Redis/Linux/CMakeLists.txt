cmake_minimum_required(VERSION 3.0)

project(RedisClient)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

link_directories("${SLIB_PATH}/external/lib/Linux/${CMAKE_SYSTEM_PROCESSOR}")

add_executable(RedisClient
  ../main.cpp
)

set_target_properties(RedisClient PROPERTIES LINK_FLAGS "${SLIB_LINK_STATIC_FLAGS}")

target_link_libraries (
  RedisClient
  slib
  hiredis
  pthread
  dl
)

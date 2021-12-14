set (SLIB_PATH "${CMAKE_CURRENT_LIST_DIR}/..")

include ("${SLIB_PATH}/build/CMake/common.cmake")

if(CMAKE_CXX_FLAGS MATCHES "-std=")
else ()
 set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

include_directories ("${SLIB_PATH}/include")
link_directories("${SLIB_LIB_PATH}")

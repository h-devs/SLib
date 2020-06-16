set (SLIB_PATH "${CMAKE_CURRENT_LIST_DIR}/../../..")

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64")
 set (SLIB_ARM YES)
 if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  set (SLIB_ARM64 YES)
 endif ()
endif ()
if (CMAKE_SYSTEM_PROCESSOR MATCHES "^x86" OR CMAKE_SYSTEM_PROCESSOR MATCHES "i[3456]86")
 set (SLIB_X86 YES)
 if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  set (SLIB_X86_64 YES)
 endif ()
endif ()

set (EXTERNAL_LIB_PATH "${SLIB_PATH}/external/lib/CMake/${CMAKE_BUILD_TYPE}-${CMAKE_SYSTEM_PROCESSOR}")
if (ANDROID)
 set (EXTERNAL_LIB_PATH "${SLIB_PATH}/external/lib/Android/${ANDROID_ABI}")
endif ()
if (CMAKE_SYSTEM_NAME STREQUAL Linux)
 set (EXTERNAL_LIB_PATH "${SLIB_PATH}/external/lib/Linux/${CMAKE_SYSTEM_PROCESSOR}")
endif ()

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -frtti")
# generates no debug information
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g0")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g0")
if (SLIB_ARM AND NOT SLIB_ARM64)
 set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -marm -mfpu=neon")
 set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -marm -mfpu=neon") 
 set (CMAKE_ASM_FLAGS, "${CMAKE_ASM_FLAGS} -marm -mfpu=neon")
endif ()

# enable asm
enable_language(ASM)
if (ANDROID AND SLIB_X86)
 enable_language (ASM_NASM)
 set (
  CMAKE_ASM_NASM_COMPILE_OBJECT
  "<CMAKE_ASM_NASM_COMPILER> <FLAGS> -o <OBJECT> <SOURCE>"
 )
endif ()

include ("${CMAKE_CURRENT_LIST_DIR}/external/yasm.cmake")
include ("${CMAKE_CURRENT_LIST_DIR}/external/freetype.cmake")
include ("${CMAKE_CURRENT_LIST_DIR}/external/sqlite.cmake")
include ("${CMAKE_CURRENT_LIST_DIR}/external/opus.cmake")
include ("${CMAKE_CURRENT_LIST_DIR}/external/vpx.cmake")
include ("${CMAKE_CURRENT_LIST_DIR}/external/zxing.cmake")
include ("${CMAKE_CURRENT_LIST_DIR}/external/hiredis.cmake")

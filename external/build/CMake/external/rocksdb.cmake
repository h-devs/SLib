set (TARGET_NAME rocksdb)
set (ROOT_DIR "${SLIB_PATH}/external/src/rocksdb")

set (
 SRC_LIST
 "${ROOT_DIR}/unity.cc"
)

include ("${CMAKE_CURRENT_LIST_DIR}/common.inc.cmake")

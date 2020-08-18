
set (LIBSAPP_CORE_FILES
 "${SLIB_PATH}/src/sapp/sapp_document.cpp"
 "${SLIB_PATH}/src/sapp/sapp_resources.cpp"
 "${SLIB_PATH}/src/sapp/sapp_util.cpp"
 "${SLIB_PATH}/src/sapp/sapp_values.cpp"
)

add_library (
 libsapp STATIC
 ${LIBSAPP_CORE_FILES}
)

set_target_properties (
 libsapp
 PROPERTIES
 ARCHIVE_OUTPUT_DIRECTORY "${SLIB_LIB_PATH}"
 ARCHIVE_OUTPUT_NAME "sapp"
)

set (SAPP_CORE_FILES
 "${SLIB_PATH}/tool/src/sapp/main.cpp"
)

add_executable (
 tool-sapp
 ${SAPP_CORE_FILES}
)

include_directories (
 "${SLIB_PATH}/src/sapp"
)

link_directories(
 "${SLIB_LIB_PATH}"
)

target_link_libraries (
 tool-sapp
 sapp
 slib
 pthread
 dl
)

set_target_properties (
 tool-sapp
 PROPERTIES
 LINK_FLAGS "-static-libgcc -Wl,--wrap=memcpy"
 RUNTIME_OUTPUT_DIRECTORY "${SLIB_PATH}/bin/${CMAKE_SYSTEM_NAME}"
 RUNTIME_OUTPUT_NAME "sapp"
)

add_executable (
 sapp
 "${SLIB_PATH}/tool/src/sapp/main.cpp"
 "${SLIB_PATH}/src/sapp/sapp_document.cpp"
)

include_directories (
 "${SLIB_PATH}/src/sapp"
)

target_link_libraries (
 sapp
 slib
 pthread
 dl
)

set_target_properties (
 sapp
 PROPERTIES
 LINK_FLAGS "${SLIB_LINK_STATIC_FLAGS}"
 RUNTIME_OUTPUT_DIRECTORY "${SLIB_PATH}/bin/${CMAKE_SYSTEM_NAME}"
 RUNTIME_OUTPUT_NAME "sapp"
)

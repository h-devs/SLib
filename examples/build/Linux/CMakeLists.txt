cmake_minimum_required(VERSION 3.0)

project(SLibExamples)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../base/ARC" "${CMAKE_BINARY_DIR}/ARC")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../base/Containers" "${CMAKE_BINARY_DIR}/Containers")

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../database/Redis/Linux" "${CMAKE_BINARY_DIR}/Redis")

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../device/GetMachineSN/Linux" "${CMAKE_BINARY_DIR}/GetMachineSN")

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../network/HttpServer" "${CMAKE_BINARY_DIR}/HttpServer")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../network/IPC" "${CMAKE_BINARY_DIR}/IPC")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../network/Ping" "${CMAKE_BINARY_DIR}/Ping")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../network/SocketEvent" "${CMAKE_BINARY_DIR}/SocketEvent")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../network/StaticWeb" "${CMAKE_BINARY_DIR}/StaticWeb")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../network/TAP/Linux" "${CMAKE_BINARY_DIR}/TAP")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../network/UdpHolepuching" "${CMAKE_BINARY_DIR}/UdpHolepuching")

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../storage/Mirror/Linux" "${CMAKE_BINARY_DIR}/Mirror")

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../ui/CameraView/Linux" "${CMAKE_BINARY_DIR}/CameraView")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../ui/ImageView/Linux" "${CMAKE_BINARY_DIR}/ImageView")

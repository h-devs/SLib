cmake_minimum_required(VERSION 3.0)

project(SLibExamples)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../Base/ARC" "${CMAKE_BINARY_DIR}/ARC")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../Base/Containers" "${CMAKE_BINARY_DIR}/Containers")

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../Database/Redis/Linux" "${CMAKE_BINARY_DIR}/Redis")

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../Device/GetMachineSN/Linux" "${CMAKE_BINARY_DIR}/GetMachineSN")

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../Network/HttpServer" "${CMAKE_BINARY_DIR}/HttpServer")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../Network/IPC" "${CMAKE_BINARY_DIR}/IPC")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../Network/Ping" "${CMAKE_BINARY_DIR}/Ping")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../Network/SocketEvent" "${CMAKE_BINARY_DIR}/SocketEvent")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../Network/StaticWeb" "${CMAKE_BINARY_DIR}/StaticWeb")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../Network/TAP/Linux" "${CMAKE_BINARY_DIR}/TAP")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../Network/UdpHolepuching" "${CMAKE_BINARY_DIR}/UdpHolepuching")

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../Storage/Mirror/Linux" "${CMAKE_BINARY_DIR}/Mirror")

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../UI/CameraView/Linux" "${CMAKE_BINARY_DIR}/CameraView")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../UI/ImageView/Linux" "${CMAKE_BINARY_DIR}/ImageView")

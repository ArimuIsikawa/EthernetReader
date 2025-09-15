cmake_minimum_required(VERSION 3.10)

project(UAV_Ethernet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-w)

include_directories(
    include/
    Mavlink_Lib/common/
    Mavlink_Lib/ardupilotmega/
)

set(SOURCES
    src/FlyPlaneData.cpp
    src/InterfaceUDP.cpp
    src/InterfaceTCP.cpp
)

set(HEADERS
    include/FlyPlaneData.h
    include/InterfaceUDP.h
    include/InterfaceTCP.h
    Mavlink_Lib/common/mavlink.h
    Mavlink_Lib/ardupilotmega/ardupilotmega.h
)

add_executable(${PROJECT_NAME} "main.cpp" ${SOURCES} ${HEADERS})
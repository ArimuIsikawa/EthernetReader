cmake_minimum_required(VERSION 3.10)

project(EthernetReader LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-w)

include_directories(
    src/FlyPlaneData/
    src/InterfaceUDP/
    Mavlink_Lib/common/
Mavlink_Lib/ardupilotmega/
)

set(SOURCES
    src/FlyPlaneData/FlyPlaneData.cpp
    src/InterfaceUDP/InterfaceUDP.cpp
)

set(HEADERS
    src/FlyPlaneData/FlyPlaneData.h
    src/InterfaceUDP/InterfaceUDP.h
    Mavlink_Lib/common/mavlink.h
    Mavlink_Lib/ardupilotmega/ardupilotmega.h
)


add_executable(${PROJECT_NAME} "main.cpp" ${SOURCES} ${HEADERS})
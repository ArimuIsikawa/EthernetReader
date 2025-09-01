cmake_minimum_required(VERSION 3.10)

project(EthernetReader LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-w)

include_directories(
    src/FlyPlaneData/
    src/InterfaceUDP/
)

set(SOURCES
    src/FlyPlaneData/FlyPlaneData.cpp
    src/InterfaceUDP/InterfaceUDP.cpp
)

set(HEADERS
    src/FlyPlaneData/FlyPlaneData.h
    src/InterfaceUDP/InterfaceUDP.h
    test.h
)


add_executable(${PROJECT_NAME}  "main.cpp" 
            ${SOURCES} ${HEADERS} ${PROTO_SRC} ${PROTO_HDRS})
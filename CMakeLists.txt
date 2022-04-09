cmake_minimum_required(VERSION 3.16)

project(wrt2pdf VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 ${QT5_MIN_VERSION} REQUIRED COMPONENTS PrintSupport)

add_executable(wrt2pdf
    src/main.cpp
)

target_link_libraries(wrt2pdf PRIVATE Qt5::PrintSupport)

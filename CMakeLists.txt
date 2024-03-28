cmake_minimum_required(VERSION 3.16)

project(wrt2pdf LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x060602)

find_package(Qt6 REQUIRED COMPONENTS PrintSupport)

qt_standard_project_setup()

# tell about our features (and what is missing)
include(FeatureSummary)
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

add_executable(wrt2pdf
    src/main.cpp
)

target_link_libraries(wrt2pdf PRIVATE Qt6::PrintSupport)

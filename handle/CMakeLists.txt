cmake_minimum_required(VERSION 3.28)
project(SimCardDeviceHandle LANGUAGES CXX)
find_package(Qt6 6.8.3 REQUIRED COMPONENTS Core)
include(FetchContent)
if(NOT TARGET Lidar::HandleCore)
    FetchContent_Declare(lidar_handle_core
        GIT_REPOSITORY https://github.com/Lateef-ZHOU/HandleCore.git
        GIT_TAG 9d74a19b91494354b3709bed04603a61ed3408bd)
    FetchContent_MakeAvailable(lidar_handle_core)
endif()
add_library(HandleSimCard INTERFACE)
add_library(Lidar::SimCard ALIAS HandleSimCard)
target_sources(HandleSimCard INTERFACE
    "${CMAKE_CURRENT_LIST_DIR}/../include/SimCardHandle.h"
    "${CMAKE_CURRENT_LIST_DIR}/../include/SimCardProtocol.h")
target_link_libraries(HandleSimCard INTERFACE Lidar::HandleCore)
target_include_directories(HandleSimCard INTERFACE "${CMAKE_CURRENT_LIST_DIR}/../include")

if(PROJECT_IS_TOP_LEVEL)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../tests/handle" tests)
    endif()
endif()

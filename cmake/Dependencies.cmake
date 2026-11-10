include(FetchContent)
set(TASKCORE_WITH_QML ON)
if(NOT TARGET Lidar::TaskUI)
    FetchContent_Declare(lidar_taskcore
        GIT_REPOSITORY https://github.com/Lateef-ZHOU/TaskCore.git
        GIT_TAG 4149454dfd04534e8d3d09e1b4b65ce7acad23b4)
    FetchContent_MakeAvailable(lidar_taskcore)
endif()

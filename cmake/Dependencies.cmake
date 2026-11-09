include(FetchContent)
set(TASKCORE_WITH_QML ON)
if(NOT TARGET Lidar::TaskUI)
    FetchContent_Declare(lidar_taskcore
        GIT_REPOSITORY https://github.com/Lateef-ZHOU/TaskCore.git
        GIT_TAG 574891563e5e1e8bec374c502162238975de9ea6)
    FetchContent_MakeAvailable(lidar_taskcore)
endif()

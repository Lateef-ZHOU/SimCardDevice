include(FetchContent)
set(TASKCORE_WITH_QML ON)
if(NOT TARGET Lidar::TaskUI)
    FetchContent_Declare(lidar_taskcore
        GIT_REPOSITORY https://github.com/Lateef-ZHOU/TaskCore.git
        GIT_TAG 1d12d27d510fa3ebedd53221f3b0b9156d2e2f1c)
    FetchContent_MakeAvailable(lidar_taskcore)
endif()

include(FetchContent)
set(TASKCORE_WITH_QML ON)
if(NOT TARGET Lidar::TaskUI)
    FetchContent_Declare(lidar_taskcore
        GIT_REPOSITORY https://github.com/Lateef-ZHOU/TaskCore.git
        GIT_TAG ed4962098dfcbcd4b3f9917735e18f471028a6f2)
    FetchContent_MakeAvailable(lidar_taskcore)
endif()

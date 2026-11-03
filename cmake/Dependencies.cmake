include(FetchContent)
set(TASKCORE_WITH_QML ON)
if(NOT TARGET Lidar::TaskUI)
    FetchContent_Declare(lidar_taskcore
        GIT_REPOSITORY https://github.com/Lateef-ZHOU/TaskCore.git
        GIT_TAG c76f8ca5bb09a2c592419eaae946d7c83d921801)
    FetchContent_MakeAvailable(lidar_taskcore)
endif()

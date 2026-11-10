include(FetchContent)
set(TASKCORE_WITH_QML ON)
if(NOT TARGET Lidar::TaskUI)
    FetchContent_Declare(lidar_taskcore
        GIT_REPOSITORY https://github.com/Lateef-ZHOU/TaskCore.git
        GIT_TAG 5eb2f2ca5911f0490248af7411711f7c35dcd183)
    FetchContent_MakeAvailable(lidar_taskcore)
endif()

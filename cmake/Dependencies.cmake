include(FetchContent)
set(TASKCORE_WITH_QML ON)
if(NOT TARGET Lidar::TaskUI)
    FetchContent_Declare(lidar_taskcore
        GIT_REPOSITORY https://github.com/Lateef-ZHOU/TaskCore.git
        GIT_TAG 7202bf3d872804e99cff2a5ad85b124cc8869eab)
    FetchContent_MakeAvailable(lidar_taskcore)
endif()

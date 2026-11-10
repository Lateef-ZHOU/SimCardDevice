include(FetchContent)
set(TASKCORE_WITH_QML ON)
if(NOT TARGET Lidar::TaskUI)
    FetchContent_Declare(lidar_taskcore
        GIT_REPOSITORY https://github.com/Lateef-ZHOU/TaskCore.git
        GIT_TAG aa0201c9c3fee1bb11fb71f9f108b77340723a61)
    FetchContent_MakeAvailable(lidar_taskcore)
endif()

include(FetchContent)
set(TASKCORE_WITH_QML ON)
if(NOT TARGET Lidar::TaskUI)
    FetchContent_Declare(lidar_taskcore
        GIT_REPOSITORY https://github.com/Lateef-ZHOU/TaskCore.git
        GIT_TAG 45722da20a7667b895a84bf59e9176e39f3802ee)
    FetchContent_MakeAvailable(lidar_taskcore)
endif()

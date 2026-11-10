include(FetchContent)
set(TASKCORE_WITH_QML ON)
if(NOT TARGET Lidar::TaskUI)
    FetchContent_Declare(lidar_taskcore
        GIT_REPOSITORY https://github.com/Lateef-ZHOU/TaskCore.git
        GIT_TAG 67f454b9b90f097dcc0c2f551eb3ddb40edd0666)
    FetchContent_MakeAvailable(lidar_taskcore)
endif()

include(FetchContent)
set(TASKCORE_WITH_QML ON)
if(NOT TARGET Lidar::TaskUI)
    FetchContent_Declare(lidar_taskcore
        GIT_REPOSITORY https://github.com/Lateef-ZHOU/TaskCore.git
        GIT_TAG 840c67b5c25da0a21e083b953486962593621e8b)
    FetchContent_MakeAvailable(lidar_taskcore)
endif()

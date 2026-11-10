include(FetchContent)
set(TASKCORE_WITH_QML ON)
if(NOT TARGET Lidar::TaskUI)
    FetchContent_Declare(lidar_taskcore
        GIT_REPOSITORY https://github.com/Lateef-ZHOU/TaskCore.git
        GIT_TAG 5c75a25bfe462830a31a2d69fe2cdd308d6f967d)
    FetchContent_MakeAvailable(lidar_taskcore)
endif()

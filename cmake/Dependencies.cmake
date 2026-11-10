include(FetchContent)
set(TASKCORE_WITH_QML ON)
if(NOT TARGET Lidar::TaskUI)
    FetchContent_Declare(lidar_taskcore
        GIT_REPOSITORY https://github.com/Lateef-ZHOU/TaskCore.git
        GIT_TAG ed8bd513b9fe3ce2b4fb9f902040a2c5765b5a33)
    FetchContent_MakeAvailable(lidar_taskcore)
endif()

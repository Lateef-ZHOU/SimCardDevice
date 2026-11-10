include(FetchContent)
set(TASKCORE_WITH_QML ON)
if(NOT TARGET Lidar::TaskUI)
    FetchContent_Declare(lidar_taskcore
        GIT_REPOSITORY https://github.com/Lateef-ZHOU/TaskCore.git
        GIT_TAG 29799b800bf937622c4818e79d8ef097b455d2bb)
    FetchContent_MakeAvailable(lidar_taskcore)
endif()

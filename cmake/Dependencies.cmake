include(FetchContent)
set(HANDLECORE_WITH_SIM_CARD ON)
if(NOT TARGET Lidar::HandleCore)
    FetchContent_Declare(lidar_handle_core
        GIT_REPOSITORY https://github.com/Lateef-ZHOU/HandleCore.git
        GIT_TAG 40b7dcac817089c4f5dfbefb20c3b39b188b9f70)
    FetchContent_MakeAvailable(lidar_handle_core)
endif()
if(NOT TARGET Lidar::SimCard)
    message(FATAL_ERROR "Enable HANDLECORE_WITH_SIM_CARD before the first HandleCore configuration")
endif()
set(TASKCORE_WITH_QML ON)
if(NOT TARGET Lidar::TaskUI)
    FetchContent_Declare(lidar_taskcore
        GIT_REPOSITORY https://github.com/Lateef-ZHOU/TaskCore.git
        GIT_TAG 9d35c7372ac514f813d96301cc9cfcb1e622db72)
    FetchContent_MakeAvailable(lidar_taskcore)
endif()

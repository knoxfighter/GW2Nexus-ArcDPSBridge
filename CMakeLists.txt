cmake_minimum_required(VERSION 3.28)

project(GW2Nexus-ArcDPSBridge)

find_package(ArcdpsExtension CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(curl REQUIRED)
find_path(NEXUS_INCLUDE_DIRS "Nexus/Nexus.h")

add_library(${PROJECT_NAME} SHARED dllmain.cpp
        EventHandler.h
        BridgeData.h)

TARGET_COMPILE_FEATURES(${PROJECT_NAME} PUBLIC cxx_std_23)

# Use -MT / -MTd runtime library
SET_PROPERTY(TARGET ${PROJECT_NAME} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(${PROJECT_NAME} PRIVATE ArcdpsExtension::ArcdpsExtension)
target_link_libraries(${PROJECT_NAME} PRIVATE magic_enum::magic_enum)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl)
target_include_directories(${PROJECT_NAME} PRIVATE ${NEXUS_INCLUDE_DIRS})

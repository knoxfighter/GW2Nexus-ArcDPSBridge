cmake_minimum_required(VERSION 3.26)
project(arcdps_nexus_bridge)

set(CMAKE_CXX_STANDARD 23)

add_compile_definitions(
        NOMINMAX
        MAGIC_ENUM_RANGE_MAX=256
        IMGUI_DEFINE_MATH_OPERATORS
)

find_package(ArcdpsExtension CONFIG REQUIRED)
FIND_PACKAGE(imgui CONFIG REQUIRED)
FIND_PACKAGE(magic_enum CONFIG REQUIRED)
FIND_PACKAGE(nlohmann_json CONFIG REQUIRED)
FIND_PACKAGE(ArcdpsUnofficialExtras CONFIG REQUIRED)
FIND_PACKAGE(CURL REQUIRED)

add_library(arcdps_nexus_bridge SHARED main.cpp
        ArcdpsEventHandler.h)

# Use -MT / -MTd runtime library
set_property(TARGET arcdps_nexus_bridge PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(${PROJECT_NAME} PRIVATE imgui::imgui)
target_link_libraries(${PROJECT_NAME} PRIVATE magic_enum::magic_enum)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} PRIVATE ArcdpsUnofficialExtras)
target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl)
target_link_libraries(arcdps_nexus_bridge PRIVATE ArcdpsExtension)

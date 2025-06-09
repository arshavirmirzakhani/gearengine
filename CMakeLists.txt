cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)


project(gear_engine)
project(gear_engine_editor)

file(GLOB_RECURSE SOURCES_ENGINE ${CMAKE_CURRENT_SOURCE_DIR}/engine_bin/*.cpp)
file(GLOB_RECURSE SOURCES_ENGINE_EDITOR ${CMAKE_CURRENT_SOURCE_DIR}/engine_editor/*.cpp)

set(COMMON_INCLUDES
    ${PROJECT_SOURCE_DIR}/engine_core
    ${PROJECT_SOURCE_DIR}/vendor/lua/lua-5.4.7/include
    ${PROJECT_SOURCE_DIR}/vendor/raylib/src
    ${PROJECT_SOURCE_DIR}/vendor/
    ${PROJECT_SOURCE_DIR}/vendor/box2d/include
    ${PROJECT_SOURCE_DIR}/engine_core
)


add_executable(gear_engine ${SOURCES_ENGINE})
add_executable(gear_engine_editor ${SOURCES_ENGINE_EDITOR}
    ${PROJECT_SOURCE_DIR}/engine_editor/imgui/
)

target_include_directories(gear_engine PRIVATE ${COMMON_INCLUDES})
target_include_directories(gear_engine_editor PRIVATE ${COMMON_INCLUDES}
    ${PROJECT_SOURCE_DIR}/engine_editor/imgui/
)

add_subdirectory(vendor/raylib)
add_subdirectory(vendor/lua)
add_subdirectory(vendor/box2d)

target_link_libraries(gear_engine PRIVATE raylib lua_static box2d)
target_link_libraries(gear_engine_editor PRIVATE raylib lua_static box2d)

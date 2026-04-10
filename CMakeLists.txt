cmake_minimum_required(VERSION 3.24)
project(game)

set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${CMAKE_BINARY_DIR}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

include(CTest)

find_package(PkgConfig REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
pkg_check_modules(EPOXY REQUIRED epoxy)

# Add all .cpp files in the src directory and its subdirectories
file(GLOB_RECURSE CPP_SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Add all .h files in the src directory and its subdirectories
file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS src/*.h)

add_executable(game ${CPP_SOURCES})

pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)
pkg_check_modules(XRANDR REQUIRED IMPORTED_TARGET xrandr)
pkg_check_modules(XINERAMA REQUIRED IMPORTED_TARGET xinerama)
pkg_check_modules(XI REQUIRED IMPORTED_TARGET xi)
pkg_check_modules(XCURSOR REQUIRED IMPORTED_TARGET xcursor)

target_link_libraries(game PRIVATE 
    glfw 
    glm::glm 
    ${EPOXY_LIBRARIES}
    PkgConfig::X11 
    PkgConfig::XRANDR 
    PkgConfig::XINERAMA 
    PkgConfig::XI 
    PkgConfig::XCURSOR
)

target_include_directories(game PRIVATE ${EPOXY_INCLUDE_DIRS})

if(BUILD_TESTING)
    add_executable(engine_tests
        tests/test_main.cpp
        src/file_manager/file_manager.cpp
        src/raycast/raycast.cpp
        src/sceneobject/sceneobject.cpp
        src/game/inventory/item_definition.cpp
        src/game/inventory/inventory_container.cpp
        src/game/inventory/inventory_system.cpp
    )

    target_link_libraries(engine_tests PRIVATE glm::glm)
    target_include_directories(engine_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

    set(ENGINE_TEST_CASES
        "AABBAndFrustum suite"
        "CameraAndRaycast suite"
        "FileItemAndLight suite"
        "InventoryContainer suite"
        "SceneObject suite"
        "InventorySystem suite"
        "UndoManager suite"
    )

    foreach(test_case IN LISTS ENGINE_TEST_CASES)
        add_test(NAME "${test_case}" COMMAND engine_tests "${test_case}")
    endforeach()
endif()

# Copy scene.json to build directory
file(COPY "${CMAKE_SOURCE_DIR}/scene.json" DESTINATION "${CMAKE_BINARY_DIR}")

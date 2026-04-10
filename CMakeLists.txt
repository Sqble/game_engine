cmake_minimum_required(VERSION 3.24)
project(game)

set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${CMAKE_BINARY_DIR}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

find_package(PkgConfig REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
pkg_check_modules(EPOXY REQUIRED epoxy)

# Add all .cpp files in the src directory and its subdirectories
file(GLOB_RECURSE CPP_SOURCES src/*.cpp)

# Add all .h files in the src directory and its subdirectories
file(GLOB_RECURSE HEADER_FILES src/*.h)

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

# Copy scene.json to build directory
file(COPY "${CMAKE_SOURCE_DIR}/scene.json" DESTINATION "${CMAKE_BINARY_DIR}")
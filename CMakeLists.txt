cmake_minimum_required(VERSION 3.26)

project(trafficlight CXX)

find_package(PkgConfig)

pkg_search_module(SDL2 IMPORTED_TARGET SDL2)
pkg_search_module(CAIRO IMPORTED_TARGET cairo)

add_executable(trafficlight trafficlight.cpp Light.cpp draw.cpp)

set_property(TARGET trafficlight PROPERTY C_STANDARD 17)
set_property(TARGET trafficlight PROPERTY CXX_STANDARD 23)

target_link_libraries(trafficlight PkgConfig::SDL2 PkgConfig::CAIRO)

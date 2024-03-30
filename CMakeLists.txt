cmake_minimum_required(VERSION 3.26)

project(trafficlight C)

find_package(PkgConfig)

pkg_search_module(SDL2 IMPORTED_TARGET SDL2)
pkg_search_module(CAIRO IMPORTED_TARGET cairo)

add_executable(trafficlight
        trafficlight.c
        draw.c
)

set_property(TARGET trafficlight PROPERTY C_STANDARD 23)

target_link_libraries(trafficlight PkgConfig::SDL2 PkgConfig::CAIRO)

cmake_minimum_required(VERSION 2.8)
project(imgui-wl)

find_package(PkgConfig)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
pkg_check_modules(WAYLAND_EGL REQUIRED wayland-egl)
pkg_check_modules(EGL REQUIRED egl)
pkg_check_modules(GLES REQUIRED glesv2)

set(SOURCES
    ../../imgui.cpp
    ../../imgui_demo.cpp
    ../../imgui_draw.cpp        
    ../../imgui_widgets.cpp
    ../../backends/imgui_impl_opengl3.cpp
    main.cpp
)

SET(CMAKE_CXX_FLAGS  ${CMAKE_CXX_FLAGS} " -DIMGUI_IMPL_OPENGL_ES2 ")

set(LIBS ${WAYLAND_CLIENT_LIBRARIES} ${WAYLAND_EGL_LIBRARIES} ${EGL_LIBRARIES} ${GLES_LIBRARIES})

add_executable(imgui-wl ${SOURCES})
target_include_directories(imgui-wl PRIVATE ../.. ../../backends)
target_link_libraries(imgui-wl ${LIBS})
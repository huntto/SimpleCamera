cmake_minimum_required(VERSION 3.10.2)

add_library(simple-camera
        SHARED
        simple_camera.cpp)


find_library(log-lib log)
find_library(camera-lib camera2ndk)
find_library(android-lib android)

target_link_libraries(
        simple-camera

        ${camera-lib}
        ${android-lib}
        ${log-lib})
cmake_minimum_required(VERSION 3.0.0)
project(dfu VERSION 0.1.0)

add_library(libdfu INTERFACE)
target_include_directories(libdfu INTERFACE inc)
target_compile_features(libdfu INTERFACE cxx_std_20)
target_compile_options(libdfu INTERFACE "-Wall" "-Wextra" "-Wpedantic")

add_executable(dfu main.cpp)
target_compile_features(dfu PRIVATE cxx_std_20)
target_link_libraries(dfu PRIVATE libdfu)

add_executable(testdfu 
    test/dec.cpp
    test/enc.cpp)
target_compile_features(testdfu PRIVATE cxx_std_20)
target_link_libraries(testdfu PRIVATE gtest_main libdfu)

enable_testing()
include(FetchContent)
FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)
gtest_discover_tests(testdfu)
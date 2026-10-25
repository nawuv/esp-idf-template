cmake_minimum_required(VERSION 3.20)
project(espkit_host_tests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(FetchContent)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG b514bdc898e2951020cbdca1304b75f5950d1f59)
FetchContent_MakeAvailable(googletest)
enable_testing()
add_executable(policy_tests
    test_timed_output.cpp
    test_workers.cpp
    ../../components/application/src/timed_output.cpp
    ../../components/application/src/workers.cpp)
target_include_directories(policy_tests PRIVATE
    ../../components/core/include
    ../../components/application/include
    ../../components/sensors/include
    ../../components/actuators/include)
target_compile_options(policy_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_link_libraries(policy_tests PRIVATE GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(policy_tests)

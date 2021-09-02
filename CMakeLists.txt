cmake_minimum_required(VERSION 3.0.0)
project(binfmt)
set(CMAKE_CXX_STANDARD 17)

add_library(binfmt binfmt.h)
set_target_properties(binfmt PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(binfmt PROPERTIES PUBLIC_HEADER "binfmt.h")

option(TESTS "Compile tests" OFF)
option(EXAMPLES "Compile examples" OFF)

if(TESTS)
    include(FetchContent)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    enable_testing()

    add_executable(binfmt_tests tests.cpp)
    add_executable(binfmt_benchmarks benchmarks.cpp)

    target_compile_definitions(binfmt_tests PRIVATE -DTESTS)
    target_compile_definitions(binfmt_benchmarks PRIVATE -DTESTS)

    target_link_libraries(binfmt_tests gtest_main)
    target_link_libraries(binfmt_benchmarks gtest_main)

    include(GoogleTest)
    gtest_discover_tests(binfmt_tests)
endif()

if(EXAMPLES)
    add_executable(binfmt_ex_pod examples/pod.cpp)
    add_executable(binfmt_ex_min examples/minimal.cpp)
endif()

install(TARGETS binfmt
        LIBRARY DESTINATION "lib"
        PUBLIC_HEADER DESTINATION "include")

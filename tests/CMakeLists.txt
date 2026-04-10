add_executable(iggen_tests smoke_test.cpp)
add_test(NAME iggen_smoke COMMAND iggen_tests)

add_executable(iggen_test_detector test_detector.cpp)
target_include_directories(iggen_test_detector PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME iggen_detector COMMAND iggen_test_detector)

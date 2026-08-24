# Shared include dir for all test targets
set(IGGEN_TEST_INCLUDES ${CMAKE_SOURCE_DIR}/include)

add_executable(iggen_smoke smoke_test.cpp)
target_include_directories(iggen_smoke PRIVATE ${IGGEN_TEST_INCLUDES})
target_link_libraries(iggen_smoke PRIVATE wcppcli httplib::httplib OpenSSL::SSL OpenSSL::Crypto)
target_compile_definitions(iggen_smoke PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
add_test(NAME iggen_smoke COMMAND iggen_smoke)

add_executable(iggen_test_detector test_detector.cpp)
target_include_directories(iggen_test_detector PRIVATE ${IGGEN_TEST_INCLUDES})
add_test(NAME iggen_detector COMMAND iggen_test_detector)

add_executable(iggen_test_file_writer test_file_writer.cpp)
target_include_directories(iggen_test_file_writer PRIVATE ${IGGEN_TEST_INCLUDES})
target_link_libraries(iggen_test_file_writer PRIVATE wcppcli)
add_test(NAME iggen_file_writer COMMAND iggen_test_file_writer)

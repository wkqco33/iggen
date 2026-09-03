# Shared include dir for all test targets
set(IGGEN_TEST_INCLUDES ${CMAKE_SOURCE_DIR}/include)

add_executable(iggen_smoke smoke_test.cpp)
target_include_directories(iggen_smoke PRIVATE ${IGGEN_TEST_INCLUDES})
target_link_libraries(iggen_smoke PRIVATE wcppcli httplib::httplib OpenSSL::SSL OpenSSL::Crypto llm_client)
target_compile_definitions(iggen_smoke PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
add_test(NAME iggen_smoke COMMAND iggen_smoke)

add_executable(iggen_test_detector test_detector.cpp)
target_include_directories(iggen_test_detector PRIVATE ${IGGEN_TEST_INCLUDES})
add_test(NAME iggen_detector COMMAND iggen_test_detector)

add_executable(iggen_test_file_writer test_file_writer.cpp)
target_include_directories(iggen_test_file_writer PRIVATE ${IGGEN_TEST_INCLUDES})
target_link_libraries(iggen_test_file_writer PRIVATE wcppcli)
add_test(NAME iggen_file_writer COMMAND iggen_test_file_writer)

add_executable(iggen_test_template_store test_template_store.cpp)
target_include_directories(iggen_test_template_store PRIVATE ${IGGEN_TEST_INCLUDES})
target_link_libraries(iggen_test_template_store PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME iggen_template_store COMMAND iggen_test_template_store)

add_executable(iggen_test_paths test_paths.cpp)
target_include_directories(iggen_test_paths PRIVATE ${IGGEN_TEST_INCLUDES})
add_test(NAME iggen_paths COMMAND iggen_test_paths)

add_executable(iggen_test_api_client test_api_client.cpp)
target_include_directories(iggen_test_api_client PRIVATE ${IGGEN_TEST_INCLUDES})
target_link_libraries(iggen_test_api_client PRIVATE httplib::httplib OpenSSL::SSL OpenSSL::Crypto)
target_compile_definitions(iggen_test_api_client PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
add_test(NAME iggen_api_client COMMAND iggen_test_api_client)

add_executable(iggen_test_project_scanner test_project_scanner.cpp)
target_include_directories(iggen_test_project_scanner PRIVATE ${IGGEN_TEST_INCLUDES})
add_test(NAME iggen_project_scanner COMMAND iggen_test_project_scanner)

add_executable(iggen_test_ai_refiner test_ai_refiner.cpp)
target_include_directories(iggen_test_ai_refiner PRIVATE ${IGGEN_TEST_INCLUDES})
target_link_libraries(iggen_test_ai_refiner PRIVATE llm_client)
add_test(NAME iggen_ai_refiner COMMAND iggen_test_ai_refiner)



# Shared include dirs for all test targets (generated headers hold IGGEN_VERSION).
set(IGGEN_TEST_INCLUDES
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_BINARY_DIR}/generated
)

# IGGEN_WARNINGS_AS_ERRORS=ON 이면 테스트 타깃에도 동일한 경고 정책을 적용한다.
set(IGGEN_TEST_WARNINGS "")
if(IGGEN_WARNINGS_AS_ERRORS)
  if(MSVC)
    set(IGGEN_TEST_WARNINGS /W4 /WX)
  else()
    set(IGGEN_TEST_WARNINGS -Wall -Wextra -Wpedantic -Werror)
  endif()
endif()

# iggen_add_test(<target> <source> [LIBS ...] [DEFINES ...])
function(iggen_add_test target source)
  cmake_parse_arguments(ARG "" "" "LIBS;DEFINES" ${ARGN})
  add_executable(${target} ${source})
  target_include_directories(${target} PRIVATE ${IGGEN_TEST_INCLUDES})
  if(ARG_LIBS)
    target_link_libraries(${target} PRIVATE ${ARG_LIBS})
  endif()
  if(ARG_DEFINES)
    target_compile_definitions(${target} PRIVATE ${ARG_DEFINES})
  endif()
  if(IGGEN_TEST_WARNINGS)
    target_compile_options(${target} PRIVATE ${IGGEN_TEST_WARNINGS})
  endif()
  add_test(NAME ${target} COMMAND ${target})
endfunction()

iggen_add_test(iggen_test_smoke smoke_test.cpp
  LIBS wcppcli httplib::httplib OpenSSL::SSL OpenSSL::Crypto llm_client
  DEFINES CPPHTTPLIB_OPENSSL_SUPPORT)

iggen_add_test(iggen_test_detector test_detector.cpp)

iggen_add_test(iggen_test_ignore_rules test_ignore_rules.cpp)

iggen_add_test(iggen_test_file_writer test_file_writer.cpp
  LIBS wcppcli)

iggen_add_test(iggen_test_template_store test_template_store.cpp
  LIBS nlohmann_json::nlohmann_json)

iggen_add_test(iggen_test_paths test_paths.cpp)

iggen_add_test(iggen_test_api_client test_api_client.cpp
  LIBS httplib::httplib OpenSSL::SSL OpenSSL::Crypto
  DEFINES CPPHTTPLIB_OPENSSL_SUPPORT)

iggen_add_test(iggen_test_project_scanner test_project_scanner.cpp)

iggen_add_test(iggen_test_ai_refiner test_ai_refiner.cpp
  LIBS llm_client)

iggen_add_test(iggen_test_report test_report.cpp
  LIBS nlohmann_json::nlohmann_json)

# CLI 계약 테스트: 목 서버(127.0.0.1)만 사용하므로 외부 네트워크가 필요 없다.
iggen_add_test(iggen_test_cli test_cli.cpp
  LIBS wcppcli httplib::httplib OpenSSL::SSL OpenSSL::Crypto nlohmann_json::nlohmann_json llm_client
  DEFINES CPPHTTPLIB_OPENSSL_SUPPORT)

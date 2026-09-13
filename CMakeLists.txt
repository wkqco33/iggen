cmake_minimum_required(VERSION 3.20)

project(iggen VERSION 0.4.0 LANGUAGES CXX)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg.json" AND NOT CMAKE_TOOLCHAIN_FILE)
  message(WARNING "vcpkg.json detected but CMAKE_TOOLCHAIN_FILE is not set. Packages might not be found. Please run cmake with -DCMAKE_TOOLCHAIN_FILE=[vcpkg_path]/scripts/buildsystems/vcpkg.cmake")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# clang-tidy를 컴파일 DB 기반으로 실행하기 위해 필요하다.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 경고를 오류로 승격하는 별도 구성(CI의 strict 빌드).
option(IGGEN_WARNINGS_AS_ERRORS "Treat iggen warnings as errors" OFF)

# 버전의 단일 소스: project(... VERSION) -> iggen_version.hpp (--version, 릴리스 태그와 동기화)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/version.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/generated/iggen_version.hpp
  @ONLY
)

if(WIN32)
  add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# HTTP and JSON dependencies
find_package(httplib CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# Dependencies fetched via git (FetchContent).
# 재현 가능한 빌드를 위해 이동하는 브랜치(master)가 아니라 커밋 SHA로 고정한다.
# 로컬 개발 중에는 -DFETCHCONTENT_SOURCE_DIR_WCPPCLI=/path/to/wcppcli 로 대체할 수 있다.
# 고정 SHA를 올릴 때는 wcppcli/LLM_client를 먼저 릴리스(또는 태그)하고 테스트를 통과시킨 뒤 바꾼다.
include(FetchContent)
FetchContent_Declare(
  wcppcli
  GIT_REPOSITORY https://github.com/wkqco33/wcppcli
  GIT_TAG 9394f8670229eb5cd68522d220b2790a6620ad2e # v0.2.0
)

set(LLM_CLIENT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LLM_CLIENT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  llm_client
  GIT_REPOSITORY https://github.com/wkqco33/LLM_client.git
  GIT_TAG 5324b70b6114897adeaab966c0c9fe309036a1eb
)

FetchContent_MakeAvailable(wcppcli llm_client)

if(TARGET llm_client_shared)
  set_target_properties(llm_client_shared PROPERTIES
    OUTPUT_NAME "llm_client_shared"
    EXCLUDE_FROM_ALL TRUE
  )
endif()

add_executable(iggen src/main.cpp)

target_include_directories(iggen PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)
target_compile_definitions(iggen PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
target_compile_features(iggen PRIVATE cxx_std_17)
# cpr/spdlog는 iggen이 직접 쓰지 않고 llm_client가 PUBLIC으로 전파한다.
# (vcpkg.json에는 llm_client가 find_package(cpr/spdlog)를 요구하므로 남겨 둔다.)
target_link_libraries(iggen PRIVATE
    wcppcli
    httplib::httplib
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    llm_client
)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

if(MSVC)
  target_compile_options(iggen PRIVATE /W4 /permissive-)
else()
  target_compile_options(iggen PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(IGGEN_WARNINGS_AS_ERRORS)
  if(MSVC)
    target_compile_options(iggen PRIVATE /WX)
  else()
    target_compile_options(iggen PRIVATE -Werror)
  endif()
endif()

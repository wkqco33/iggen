cmake_minimum_required(VERSION 3.20)

project(iggen LANGUAGES CXX)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg.json" AND NOT CMAKE_TOOLCHAIN_FILE)
  message(WARNING "vcpkg.json detected but CMAKE_TOOLCHAIN_FILE is not set. Packages might not be found. Please run cmake with -DCMAKE_TOOLCHAIN_FILE=[vcpkg_path]/scripts/buildsystems/vcpkg.cmake")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# wcppcli submodule — EXCLUDE_FROM_ALL skips example targets; wcppcli lib is
# built on-demand when iggen links against it.
add_subdirectory(wcppcli EXCLUDE_FROM_ALL)

# HTTP client (HTTPS via OpenSSL)
find_package(httplib CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(iggen src/main.cpp)

target_include_directories(iggen PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_definitions(iggen PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
target_compile_features(iggen PRIVATE cxx_std_17)
target_link_libraries(iggen PRIVATE wcppcli httplib::httplib OpenSSL::SSL OpenSSL::Crypto nlohmann_json::nlohmann_json)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

if(MSVC)
  target_compile_options(iggen PRIVATE /W4 /permissive-)
else()
  target_compile_options(iggen PRIVATE -Wall -Wextra -Wpedantic)
endif()

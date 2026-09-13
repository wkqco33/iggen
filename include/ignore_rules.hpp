#pragma once

#include <algorithm>
#include <array>
#include <string_view>

namespace iggen {

// 디렉터리 제외 규칙의 단일 소스. 언어 감지(detector)와 AI 컨텍스트 스캔
// (project_scanner)이 서로 다른 목록을 쓰면 감지 결과와 AI에게 주는 정보가
// 어긋나므로, 두 곳 모두 이 헤더를 사용한다.
//
// 목록은 is_ignored_directory()의 이진 탐색을 위해 사전순으로 유지해야 한다.
// (tests/test_ignore_rules.cpp가 정렬 상태를 검증한다.)
inline constexpr std::array<std::string_view, 34> kIgnoredDirectoryNames = {
    ".cache",      ".dart_tool",
    ".git",        ".gradle",
    ".hg",         ".idea",
    ".mypy_cache", ".next",
    ".nuxt",       ".pytest_cache",
    ".ruff_cache", ".svn",
    ".terraform",  ".tox",
    ".venv",       ".vs",
    "CMakeFiles",  "DerivedData",
    "Pods",        "__pycache__",
    "bin",         "bower_components",
    "build",       "builds",
    "coverage",    "dist",
    "env",         "node_modules",
    "obj",         "out",
    "target",      "vcpkg_installed",
    "vendor",      "venv",
};

inline auto is_ignored_directory(std::string_view name) -> bool {
    return std::binary_search(kIgnoredDirectoryNames.begin(), kIgnoredDirectoryNames.end(), name);
}

// CMake/IDE가 만드는 빌드 트리 (예: cmake-build-debug, cmake-build-release).
inline auto is_cmake_build_directory(std::string_view name) -> bool {
    return name.rfind("cmake-build-", 0) == 0;
}

inline auto is_hidden_directory(std::string_view name) -> bool {
    return !name.empty() && name.front() == '.';
}

} // namespace iggen

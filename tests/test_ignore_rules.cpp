#include "ignore_rules.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>

// detector와 project_scanner가 공유하는 제외 규칙. 두 스캐너가 서로 다른 목록을
// 쓰면 감지 결과와 AI 컨텍스트가 어긋나므로, 목록 자체를 여기서 검증한다.
namespace {

void test_list_is_sorted_and_unique() {
    // is_ignored_directory()가 이진 탐색을 쓰므로 정렬이 깨지면 조용히 오작동한다.
    assert(
        std::is_sorted(iggen::kIgnoredDirectoryNames.begin(), iggen::kIgnoredDirectoryNames.end()));
    assert(std::adjacent_find(iggen::kIgnoredDirectoryNames.begin(),
                              iggen::kIgnoredDirectoryNames.end()) ==
           iggen::kIgnoredDirectoryNames.end());
}

void test_ignored_directories() {
    assert(iggen::is_ignored_directory("node_modules"));
    assert(iggen::is_ignored_directory("vcpkg_installed"));
    assert(iggen::is_ignored_directory("build"));
    assert(iggen::is_ignored_directory(".venv"));
    assert(iggen::is_ignored_directory(".git"));
    assert(iggen::is_ignored_directory("__pycache__"));
    assert(iggen::is_ignored_directory("CMakeFiles"));

    assert(!iggen::is_ignored_directory("src"));
    assert(!iggen::is_ignored_directory("include"));
    assert(!iggen::is_ignored_directory(""));
    assert(!iggen::is_ignored_directory("build-scripts"));
}

void test_cmake_build_and_hidden_predicates() {
    assert(iggen::is_cmake_build_directory("cmake-build-debug"));
    assert(iggen::is_cmake_build_directory("cmake-build-release"));
    assert(!iggen::is_cmake_build_directory("cmake"));
    assert(!iggen::is_cmake_build_directory("build"));

    assert(iggen::is_hidden_directory(".github"));
    assert(iggen::is_hidden_directory(".git"));
    assert(!iggen::is_hidden_directory("src"));
    assert(!iggen::is_hidden_directory(""));
}

} // namespace

auto main() -> int {
    test_list_is_sorted_and_unique();
    test_ignored_directories();
    test_cmake_build_and_hidden_predicates();
    std::cout << "All ignore_rules tests passed.\n";
    return 0;
}

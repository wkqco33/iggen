#include "detector.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

// Creates a temp dir with the given relative paths (files) and returns its path.
fs::path make_tree(const std::vector<std::string> &files) {
    static int counter = 0;
    fs::path dir = fs::temp_directory_path() / ("iggen_test_" + std::to_string(counter++));
    fs::remove_all(dir);
    fs::create_directories(dir);
    for (const auto &rel : files) {
        fs::path p = dir / rel;
        fs::create_directories(p.parent_path());
        std::ofstream(p).close();
    }
    return dir;
}

void test_extension_map() {
    const auto &map = iggen::extension_map();

    // C / C++
    assert(map.at("cpp") == "c++");
    assert(map.at("cc") == "c++");
    assert(map.at("hpp") == "c++");
    assert(map.at("c") == "c");
    assert(map.at("h") == "c");

    // Common languages
    assert(map.at("rs") == "rust");
    assert(map.at("go") == "go");
    assert(map.at("java") == "java");
    assert(map.at("py") == "python");
    assert(map.at("ts") == "node");
    assert(map.at("js") == "node");
    assert(map.at("cmake") == "cmake");
    assert(map.at("tf") == "terraform");
    assert(map.at("sh") == "linux");
    assert(map.at("ps1") == "windows");

    // Unknown / empty extension should not be present
    assert(map.find("xyz") == map.end());
    assert(map.find("") == map.end());
}

void test_detect_languages() {
    auto dir = make_tree({"src/main.cpp", "src/util.rs", "app.py", "README.md"});
    auto langs = iggen::detect_languages(dir);
    fs::remove_all(dir);

    assert(langs.count("c++") == 1);
    assert(langs.count("rust") == 1);
    assert(langs.count("python") == 1);
    // README.md has no recognized extension
    assert(langs.size() == 3);
}

void test_detect_skips_build_dirs() {
    auto dir = make_tree({"build/out.cpp", "node_modules/x.js", ".git/config", "src/main.go"});
    auto langs = iggen::detect_languages(dir);
    fs::remove_all(dir);

    // Files under build/, node_modules/, .git/ must be ignored.
    assert(langs.count("c++") == 0);
    assert(langs.count("node") == 0);
    assert(langs.count("go") == 1);
    assert(langs.size() == 1);
}

void test_detect_skips_dependency_and_ide_dirs() {
    auto dir = make_tree({"vcpkg_installed/x64/include/dep.hpp", "cmake-build-debug/generated.go",
                          ".venv/lib/mod.py", "src/main.cpp"});
    auto langs = iggen::detect_languages(dir);
    fs::remove_all(dir);

    // 의존성/빌드/가상환경 디렉터리는 스캔에서 제외된다.
    assert(langs.count("go") == 0);
    assert(langs.count("python") == 0);
    assert(langs.count("c++") == 1);
    assert(langs.size() == 1);
}

} // namespace

auto main() -> int {
    test_extension_map();
    test_detect_languages();
    test_detect_skips_build_dirs();
    test_detect_skips_dependency_and_ide_dirs();
    std::cout << "All detector tests passed.\n";
    return 0;
}

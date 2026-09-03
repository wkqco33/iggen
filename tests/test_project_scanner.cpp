#include "project_scanner.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

namespace {

fs::path create_temp_fixture() {
    static int counter = 0;
    fs::path base = fs::temp_directory_path() / ("iggen_scanner_test_" + std::to_string(counter++));
    fs::remove_all(base);
    fs::create_directories(base / "src");
    fs::create_directories(base / "include");
    fs::create_directories(base / "build" / "temp");
    fs::create_directories(base / ".git" / "objects");
    fs::create_directories(base / "node_modules" / "pkg");

    // Root build files
    { std::ofstream(base / "CMakeLists.txt") << "cmake_minimum_required(VERSION 3.20)\n"; }
    { std::ofstream(base / ".env") << "API_KEY=123\n"; }

    // Source files
    { std::ofstream(base / "src" / "main.cpp") << "int main() {}\n"; }
    { std::ofstream(base / "src" / "utils.cpp") << "void util() {}\n"; }
    { std::ofstream(base / "include" / "utils.hpp") << "#pragma once\n"; }

    // Ignored directory files
    { std::ofstream(base / "build" / "temp" / "artifact.o") << "binary\n"; }
    { std::ofstream(base / ".git" / "objects" / "commit.txt") << "git\n"; }
    { std::ofstream(base / "node_modules" / "pkg" / "index.js") << "console.log();\n"; }

    return base;
}

void test_scanner_ignores_blacklisted_directories() {
    auto base = create_temp_fixture();
    auto ctx = iggen::scan_project_context(base);

    // .git, build, node_modules should NOT be in the sample tree
    for (const auto &rel_path : ctx.sample_file_tree) {
        assert(rel_path.find(".git") == std::string::npos);
        assert(rel_path.find("build") == std::string::npos);
        assert(rel_path.find("node_modules") == std::string::npos);
    }

    // .o and .js inside ignored directories should NOT be counted
    assert(ctx.extension_counts.find(".o") == ctx.extension_counts.end());
    assert(ctx.extension_counts.find(".js") == ctx.extension_counts.end());

    fs::remove_all(base);
}

void test_scanner_detects_build_and_config_files() {
    auto base = create_temp_fixture();
    auto ctx = iggen::scan_project_context(base);

    bool found_cmake = false;
    bool found_env = false;
    for (const auto &file : ctx.detected_build_files) {
        if (file == "CMakeLists.txt") {
            found_cmake = true;
        }
        if (file == ".env") {
            found_env = true;
        }
    }
    assert(found_cmake);
    assert(found_env);

    fs::remove_all(base);
}

void test_scanner_extension_counts() {
    auto base = create_temp_fixture();
    auto ctx = iggen::scan_project_context(base);

    assert(ctx.extension_counts[".cpp"] == 2);
    assert(ctx.extension_counts[".hpp"] == 1);

    fs::remove_all(base);
}

} // namespace

auto main() -> int {
    test_scanner_ignores_blacklisted_directories();
    test_scanner_detects_build_and_config_files();
    test_scanner_extension_counts();
    std::cout << "All project_scanner tests passed.\n";
    return 0;
}

#include "detector.hpp"
#include <cassert>
#include <iostream>

auto main() -> int {
    const auto& map = iggen::extension_map();

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

    // Unknown extension should not be present
    assert(map.find("xyz") == map.end());
    assert(map.find("") == map.end());

    std::cout << "All detector tests passed.\n";
    return 0;
}

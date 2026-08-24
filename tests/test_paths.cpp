#include "paths.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

auto main() -> int {
    // The user data dir must resolve to a non-empty absolute path.
    auto dir = iggen::user_data_dir();
    assert(!dir.empty());
    assert(dir.is_absolute());

    // The default cache path is the templates.json under the data dir.
    auto cache = iggen::default_cache_path();
    assert(cache == dir / "templates.json");
    assert(cache.filename() == "templates.json");

    std::cout << "All paths tests passed.\n";
    return 0;
}

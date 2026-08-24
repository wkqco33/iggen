// Smoke test: verifies all public headers compile and link together.
#include "api_client.hpp"
#include "detector.hpp"
#include "file_writer.hpp"

#include <cassert>
#include <iostream>

auto main() -> int {
    // Header-only modules must be usable together.
    assert(iggen::extension_map().size() > 0);
    assert(iggen::WriteResult::Written != iggen::WriteResult::Error);
    std::cout << "Smoke test passed.\n";
    return 0;
}

// Smoke test: verifies all public headers compile and link together.
#include "ai_refiner.hpp"
#include "api_client.hpp"
#include "detector.hpp"
#include "file_writer.hpp"
#include "paths.hpp"
#include "project_scanner.hpp"
#include "template_store.hpp"

#include <cassert>
#include <iostream>

auto main() -> int {
    // Header-only modules must be usable together.
    assert(iggen::extension_map().size() > 0);
    assert(iggen::WriteResult::Written != iggen::WriteResult::Error);
    iggen::AiConfig cfg;
    assert(cfg.provider == "ollama");
    std::cout << "Smoke test passed.\n";
    return 0;
}

// Smoke test: verifies all public headers compile and link together.
#include "ai_refiner.hpp"
#include "api_client.hpp"
#include "cli.hpp"
#include "detector.hpp"
#include "exit_code.hpp"
#include "file_writer.hpp"
#include "ignore_rules.hpp"
#include "paths.hpp"
#include "project_scanner.hpp"
#include "report.hpp"
#include "secret_input.hpp"
#include "template_store.hpp"

#include <cassert>
#include <iostream>
#include <string>

auto main() -> int {
    // Header-only modules must be usable together.
    assert(iggen::extension_map().size() > 0);
    assert(iggen::WriteResult::Written != iggen::WriteResult::Error);
    assert(iggen::to_int(iggen::ExitCode::UsageError) == 2);
    assert(iggen::is_ignored_directory("node_modules"));
    assert(std::string(IGGEN_VERSION).find('.') != std::string::npos);
    iggen::AiConfig cfg;
    assert(cfg.provider == "ollama");
    std::cout << "Smoke test passed.\n";
    return 0;
}

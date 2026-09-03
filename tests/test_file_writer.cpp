#include "file_writer.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

namespace {

fs::path temp_file() {
    static int counter = 0;
    fs::path p = fs::temp_directory_path() / ("iggen_writer_" + std::to_string(counter++));
    fs::remove(p);
    return p;
}

void test_write_new_file() {
    auto p = temp_file();
    auto r = iggen::write_output(p, "hello\n");
    assert(r == iggen::WriteResult::Written);

    std::ifstream in(p);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    assert(content == "hello\n");
    fs::remove(p);
}

void test_write_existing_non_tty_skips() {
    auto p = temp_file();
    std::ofstream(p) << "existing";
    // In a non-interactive test environment, overwriting an existing file is refused.
    auto r = iggen::write_output(p, "new");
    assert(r == iggen::WriteResult::Skipped);
    fs::remove(p);
}

void test_write_dry_run_does_not_create_file() {
    auto p = temp_file();
    std::ostringstream captured;
    auto r = iggen::write_output(p, "sample content\n", /*dry_run=*/true, captured);

    assert(r == iggen::WriteResult::DryRun);
    assert(!fs::exists(p));
    assert(captured.str() == "sample content\n");
}

} // namespace

auto main() -> int {
    test_write_new_file();
    test_write_existing_non_tty_skips();
    test_write_dry_run_does_not_create_file();
    std::cout << "All file_writer tests passed.\n";
    return 0;
}

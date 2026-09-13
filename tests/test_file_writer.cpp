#include "file_writer.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "wcppcli/wui.hpp"

namespace fs = std::filesystem;

namespace {

fs::path temp_file() {
    static int counter = 0;
    fs::path p = fs::temp_directory_path() / ("iggen_writer_" + std::to_string(counter++));
    fs::remove(p);
    return p;
}

auto read_file(const fs::path &p) -> std::string {
    std::ifstream in(p);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

void test_write_new_file() {
    auto p = temp_file();
    auto r = iggen::write_output(p, "hello\n");
    assert(r == iggen::WriteResult::Written);
    assert(read_file(p) == "hello\n");
    fs::remove(p);
}

void test_write_existing_non_interactive_needs_confirmation() {
    auto p = temp_file();
    std::ofstream(p) << "existing";

    // 비대화형 여부를 명시적으로 고정한다. 러너에 따라 stdin이 tty일 수 있어
    // (Windows CI가 그렇다) 자동 판별에 의존하면 프롬프트에서 멈출 수 있다.
    wcppcli::ui::set_interactive_enabled(false);
    std::ostringstream prompt_sink;
    auto *old_err = std::cerr.rdbuf(prompt_sink.rdbuf());
    auto r = iggen::write_output(p, "new");
    std::cerr.rdbuf(old_err);
    wcppcli::ui::reset_interactive_enabled();

    // 비대화형에서는 조용히 건너뛰지 않고 NeedsConfirmation을 돌려준다.
    // (호출자가 0이 아닌 종료 코드로 사용자에게 -y/--yes를 안내해야 한다.)
    assert(r == iggen::WriteResult::NeedsConfirmation);
    assert(read_file(p) == "existing");
    assert(prompt_sink.str().empty());
    fs::remove(p);
}

void test_force_overwrite_writes_without_prompt() {
    auto p = temp_file();
    std::ofstream(p) << "existing";
    std::ostringstream sink;
    auto r = iggen::write_output(p, "new", /*dry_run=*/false, sink, /*force_overwrite=*/true);
    assert(r == iggen::WriteResult::Written);
    assert(read_file(p) == "new");
    fs::remove(p);
}

void test_declined_prompt_keeps_existing_file() {
    auto p = temp_file();
    std::ofstream(p) << "existing";

    wcppcli::ui::set_interactive_enabled(true);
    std::istringstream answers("n\n");
    std::ostringstream prompt_sink;
    auto *old_in = std::cin.rdbuf(answers.rdbuf());
    auto *old_err = std::cerr.rdbuf(prompt_sink.rdbuf());

    auto r = iggen::write_output(p, "new");

    std::cin.rdbuf(old_in);
    std::cerr.rdbuf(old_err);
    wcppcli::ui::reset_interactive_enabled();

    assert(r == iggen::WriteResult::Skipped);
    assert(read_file(p) == "existing");
    // 프롬프트는 stdout이 아니라 stderr로 나간다.
    assert(prompt_sink.str().find("already exists") != std::string::npos);
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

void test_write_to_unwritable_path_is_error() {
    auto dir = fs::temp_directory_path() / "iggen_writer_missing_dir";
    fs::remove_all(dir);
    auto r = iggen::write_output(dir / "nested" / ".gitignore", "x");
    assert(r == iggen::WriteResult::Error);
}

} // namespace

auto main() -> int {
    test_write_new_file();
    test_write_existing_non_interactive_needs_confirmation();
    test_force_overwrite_writes_without_prompt();
    test_declined_prompt_keeps_existing_file();
    test_write_dry_run_does_not_create_file();
    test_write_to_unwritable_path_is_error();
    std::cout << "All file_writer tests passed.\n";
    return 0;
}

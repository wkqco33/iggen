#include "report.hpp"
#include "secret_input.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

namespace {

fs::path temp_file(const std::string &name) {
    static int counter = 0;
    fs::path p =
        fs::temp_directory_path() / ("iggen_report_" + name + "_" + std::to_string(counter++));
    fs::remove(p);
    return p;
}

// ---------------------------------------------------------------- report

void test_report_json_contract() {
    iggen::RunReport report;
    report.status = "ok";
    report.dry_run = true;
    report.output = ".gitignore";
    report.templates = {"python", "linux"};
    report.source = "api";
    report.content = "# python\n";

    const auto parsed = nlohmann::json::parse(iggen::to_json(report));
    assert(parsed.at("status") == "ok");
    assert(parsed.at("dry_run") == true);
    assert(parsed.at("output") == ".gitignore");
    assert(parsed.at("templates") == nlohmann::json::array({"python", "linux"}));
    assert(parsed.at("source") == "api");
    assert(parsed.at("content") == "# python\n");
    // 채우지 않은 선택 필드는 출력하지 않는다.
    assert(!parsed.contains("missing"));
    assert(!parsed.contains("error"));
    assert(!parsed.contains("fetched"));
}

void test_report_error_fields() {
    iggen::RunReport report;
    report.status = "error";
    report.missing = {"flutter"};
    report.error = "no content";

    const auto parsed = nlohmann::json::parse(iggen::to_json(report));
    assert(parsed.at("status") == "error");
    assert(parsed.at("missing") == nlohmann::json::array({"flutter"}));
    assert(parsed.at("error") == "no content");
}

void test_report_update_fields() {
    iggen::RunReport report;
    report.status = "partial";
    report.fetched = 4;
    report.failed = 1;
    report.cache_path = "/tmp/iggen/templates.json";

    const auto parsed = nlohmann::json::parse(iggen::to_json(report));
    assert(parsed.at("fetched") == 4);
    assert(parsed.at("failed") == 1);
    assert(parsed.at("cache_path") == "/tmp/iggen/templates.json");
}

// ---------------------------------------------------------------- secret input

void test_read_secret_file_trims_trailing_newline() {
    auto p = temp_file("lf");
    {
        std::ofstream out(p);
        out << "sk-test-key\n";
    }
    std::string value;
    assert(iggen::read_secret_file(p, value));
    assert(value == "sk-test-key");
    fs::remove(p);

    auto crlf = temp_file("crlf");
    {
        std::ofstream out(crlf);
        out << "sk-test-key\r\n";
    }
    value.clear();
    assert(iggen::read_secret_file(crlf, value));
    assert(value == "sk-test-key");
    fs::remove(crlf);
}

void test_read_secret_file_failures() {
    std::string value = "unchanged";
    assert(!iggen::read_secret_file(fs::temp_directory_path() / "iggen_missing_key", value));
    assert(value == "unchanged");

    auto empty = temp_file("empty");
    {
        std::ofstream out(empty);
        out << "\n";
    }
    assert(!iggen::read_secret_file(empty, value));
    fs::remove(empty);
}

void test_read_secret_stdin() {
    std::istringstream input("sk-from-stdin\n");
    auto *old = std::cin.rdbuf(input.rdbuf());
    std::string value;
    const bool ok = iggen::read_secret_stdin(value);
    std::cin.rdbuf(old);

    assert(ok);
    assert(value == "sk-from-stdin");
}

} // namespace

auto main() -> int {
    test_report_json_contract();
    test_report_error_fields();
    test_report_update_fields();
    test_read_secret_file_trims_trailing_newline();
    test_read_secret_file_failures();
    test_read_secret_stdin();
    std::cout << "All report/secret_input tests passed.\n";
    return 0;
}

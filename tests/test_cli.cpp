// CLI 계약 테스트: iggen::run()을 프로세스 내에서 호출해 종료 코드, stdout/stderr 분리,
// 기계 판독 출력을 검증한다. HTTP는 127.0.0.1 목 서버만 사용하므로 외부 네트워크에
// 의존하지 않는다.
#include "cli.hpp"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace {

// ---------------------------------------------------------------- env helpers

void set_env(const char *name, const std::string &value) {
#ifdef _WIN32
    _putenv_s(name, value.c_str());
#else
    setenv(name, value.c_str(), 1);
#endif
}

// 사용자 캐시가 개발자의 실제 홈을 건드리지 않도록 플랫폼별 데이터 디렉터리를 덮어쓴다.
void set_data_dir(const fs::path &dir) {
#ifdef _WIN32
    set_env("APPDATA", dir.string());
#elif defined(__APPLE__)
    set_env("HOME", dir.string());
#else
    set_env("XDG_DATA_HOME", dir.string());
#endif
}

fs::path make_temp_dir(const std::string &name) {
    static int counter = 0;
    fs::path dir =
        fs::temp_directory_path() / ("iggen_cli_" + name + "_" + std::to_string(counter++));
    fs::remove_all(dir);
    fs::create_directories(dir);
    return dir;
}

// 실수로 실제 gitignore.io에 요청이 나가면 테스트가 네트워크에 의존하게 되므로,
// 네트워크를 타는 모든 케이스 시작에서 목 서버 주소인지 확인한다.
void assert_local_api() {
    const char *base = std::getenv("IGGEN_API_BASE_URL");
    assert(base != nullptr);
    assert(std::string(base).rfind("http://127.0.0.1:", 0) == 0);
}

// ---------------------------------------------------------------- run harness

struct RunResult {
    int code = -1;
    std::string out;
    std::string err;
};

// 테스트 하네스는 항상 --no-input을 붙인다. 러너에 tty가 있고 출력 파일이 이미 있으면
// 프롬프트가 stdin을 기다리며 CI 작업을 몇 시간씩 붙잡을 수 있기 때문이다.
// (대화형 경로 자체는 test_file_writer.cpp에서 명시적으로 검증한다.)
auto run_cli(const std::vector<std::string> &args) -> RunResult {
    std::vector<char *> argv;
    argv.push_back(const_cast<char *>("iggen"));
    const std::string no_input = "--no-input";
    argv.push_back(const_cast<char *>(no_input.c_str()));
    for (const auto &a : args) {
        argv.push_back(const_cast<char *>(a.c_str()));
    }

    std::ostringstream out_buf;
    std::ostringstream err_buf;
    auto *old_out = std::cout.rdbuf(out_buf.rdbuf());
    auto *old_err = std::cerr.rdbuf(err_buf.rdbuf());
    const int code = iggen::run(static_cast<int>(argv.size()), argv.data());
    std::cout.rdbuf(old_out);
    std::cerr.rdbuf(old_err);

    return {code, out_buf.str(), err_buf.str()};
}

// ---------------------------------------------------------------- mock API

class MockApi {
  public:
    bool start() {
        svr_.Get("/developers/gitignore/api/list",
                 [this](const httplib::Request &, httplib::Response &res) {
                     ++requests;
                     res.status = list_status;
                     res.set_content(list_body, "text/plain");
                 });
        svr_.Get(R"(/developers/gitignore/api/(.+))",
                 [this](const httplib::Request &req, httplib::Response &res) {
                     ++requests;
                     const std::string target = req.matches[1].str();
                     if (templates.count(target) > 0) {
                         res.status = template_status;
                         res.set_content(templates[target], "text/plain");
                         return;
                     }
                     // fetch_gitignore()는 "python,linux"처럼 쉼표로 합쳐 한 번에 요청한다.
                     std::string combined;
                     std::istringstream stream(target);
                     std::string name;
                     while (std::getline(stream, name, ',')) {
                         if (templates.count(name) == 0) {
                             res.status = 404;
                             res.set_content("Not Found", "text/plain");
                             return;
                         }
                         combined += templates[name];
                     }
                     res.status = template_status;
                     res.set_content(combined, "text/plain");
                 });

        port = svr_.bind_to_any_port("127.0.0.1");
        if (port <= 0) {
            return false;
        }
        thread = std::thread([this] { svr_.listen_after_bind(); });

        // wait_until_ready()는 준비되지 않으면 무한 대기하므로, 시간 제한을 두고
        // 실패 시 조용히 멈추지 않고 원인을 출력한다.
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!svr_.is_running() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (!svr_.is_running()) {
            std::cerr << "mock API server did not become ready on 127.0.0.1:" << port << std::endl;
            svr_.stop();
            if (thread.joinable()) {
                thread.join();
            }
            return false;
        }
        return true;
    }

    ~MockApi() {
        svr_.stop();
        if (thread.joinable()) {
            thread.join();
        }
    }

    auto base_url() const -> std::string {
        return "http://127.0.0.1:" + std::to_string(port);
    }

    httplib::Server svr_;
    std::thread thread;
    int port = 0;
    std::atomic<int> requests{0};
    std::string list_body = "python,node";
    int list_status = 200;
    int template_status = 200;
    std::map<std::string, std::string> templates = {
        {"python", "# python\n__pycache__/\n"},
        {"node", "# node\nnode_modules/\n"},
        {"linux", "# linux\n"},
        {"macos", "# macos\n"},
        {"visualstudiocode", "# vscode\n"},
        {"windows", "# windows\n"},
    };
};

constexpr const char *kPythonOnly = "# python\n__pycache__/\n";

// ---------------------------------------------------------------- test cases

void test_version_flag() {
    const auto r = run_cli({"--version"});
    assert(r.code == 0);
    assert(r.out.find(IGGEN_VERSION) != std::string::npos);
    // 버전 출력은 stdout, 상태 로그는 남지 않는다.
    assert(r.err.empty());
}

void test_help_lists_docs_and_exit_codes() {
    const auto r = run_cli({"--help"});
    assert(r.code == 0);
    assert(r.out.find("Usage:") != std::string::npos);
    assert(r.out.find("https://github.com/wkqco33/iggen") != std::string::npos);
    assert(r.out.find("Exit codes") != std::string::npos);
}

void test_dry_run_keeps_stdout_clean(MockApi &api) {
    assert_local_api();
    const auto r = run_cli({"--dry-run", "-l", "python", "--no-defaults"});
    assert(r.code == 0);
    // dry-run 결과는 stdout에만, 상태 메시지는 stderr에만 나온다.
    assert(r.out == kPythonOnly);
    assert(r.err.find("[INFO") != std::string::npos);
    assert(r.err.find("__pycache__") == std::string::npos);
    assert(api.requests.load() > 0);
}

void test_quiet_silences_stderr() {
    const auto r = run_cli({"--dry-run", "--quiet", "-l", "python", "--no-defaults"});
    assert(r.code == 0);
    assert(r.out == kPythonOnly);
    assert(r.err.empty());
}

void test_json_dry_run_is_machine_readable() {
    const auto r = run_cli({"--json", "--dry-run", "-l", "python", "--no-defaults"});
    assert(r.code == 0);

    const auto parsed = nlohmann::json::parse(r.out); // 실패하면 파싱 불가능한 출력
    assert(parsed.at("status") == "ok");
    assert(parsed.at("dry_run") == true);
    assert(parsed.at("source") == "api");
    assert(parsed.at("content") == kPythonOnly);
    assert(parsed.at("templates") == nlohmann::json::array({"python"}));
    assert(r.err.find("[INFO") != std::string::npos);
}

void test_invalid_template_name_is_rejected_before_network(MockApi &api) {
    assert_local_api();
    const int before = api.requests.load();
    const std::vector<std::string> invalid = {"../etc", "python/../x", "a b", std::string(70, 'x')};
    for (const auto &bad : invalid) {
        const auto r = run_cli({"--dry-run", "-l", bad, "--no-defaults"});
        assert(r.code == 2);
        assert(r.out.empty());
    }
    assert(api.requests.load() == before);
}

void test_usage_errors_return_exit_code_2() {
    const auto unknown_flag = run_cli({"--nope"});
    assert(unknown_flag.code == 2);

    const auto positional = run_cli({"extra-arg", "--dry-run"});
    assert(positional.code == 2);
    assert(positional.err.find("unexpected argument") != std::string::npos);

    const auto missing_value = run_cli({"--output"});
    assert(missing_value.code == 2);

    const auto update_flag = run_cli({"update", "--nope"});
    assert(update_flag.code == 2);
}

void test_existing_file_needs_confirmation_without_tty() {
    const auto dir = make_temp_dir("overwrite");
    const auto target = dir / ".gitignore";
    std::ofstream(target) << "old content\n";

    // 비대화형에서 조용히 넘어가지 않고 사용법 오류(2)로 실패한다.
    // (프롬프트로 멈추지 않도록 하네스가 --no-input을 항상 붙인다.)
    const auto refused = run_cli({"-o", target.string(), "-l", "python", "--no-defaults"});
    assert(refused.code == 2);
    assert(refused.err.find("-y/--yes") != std::string::npos);
    {
        std::ifstream in(target);
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        assert(content == "old content\n");
    }

    // -y/--yes 로 덮어쓸 수 있다.
    const auto forced = run_cli({"-y", "-o", target.string(), "-l", "python", "--no-defaults"});
    assert(forced.code == 0);
    {
        std::ifstream in(target);
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        assert(content == kPythonOnly);
    }

    fs::remove_all(dir);
}

void test_legacy_dry_run_alias_warns() {
    const auto r = run_cli({"-d", "-l", "python", "--no-defaults"});
    assert(r.code == 0);
    assert(r.out == kPythonOnly);
    assert(r.err.find("deprecated") != std::string::npos);
}

void test_api_key_file_missing_is_usage_error() {
    const auto r = run_cli({"--ai", "--ai-api-key-file", "/nonexistent/iggen/key", "--dry-run",
                            "-l", "python", "--no-defaults"});
    assert(r.code == 2);
    assert(r.err.find("could not read API key file") != std::string::npos);
}

void test_api_failure_falls_back_to_builtin(MockApi &api) {
    assert_local_api();
    api.template_status = 500;
    const auto r = run_cli({"--json", "--dry-run", "-l", "python", "--no-defaults"});
    api.template_status = 200;

    assert(r.code == 0);
    const auto parsed = nlohmann::json::parse(r.out);
    assert(parsed.at("source") == "local");
    assert(parsed.at("content").get<std::string>().find("*.py[cod]") != std::string::npos);
}

void test_unknown_template_without_local_fallback_is_network_error(const MockApi &api) {
    // 아무도 듣지 않는 포트로 보내 연결 실패를 만든다.
    set_env("IGGEN_API_BASE_URL", "http://127.0.0.1:1");
    const auto r = run_cli({"--dry-run", "-l", "definitelynotatemplate", "--no-defaults"});
    set_env("IGGEN_API_BASE_URL", api.base_url()); // 이후 테스트가 실제 API를 타지 않도록 복구

    assert(r.code == 3);
    assert(r.err.find("No .gitignore content could be produced") != std::string::npos);
}

void test_update_writes_cache_and_returns_zero(MockApi &api) {
    assert_local_api();
    const auto data_dir = make_temp_dir("update_ok");
    set_data_dir(data_dir);
    api.list_body = "python,node\nlinux,macos"; // gitignore.io처럼 줄바꿈으로 감싼 목록

    const auto r = run_cli({"update", "--json"});

    assert(r.code == 0);
    const auto parsed = nlohmann::json::parse(r.out);
    assert(parsed.at("status") == "ok");
    // 줄바꿈으로 감싼 목록도 4개 템플릿으로 정확히 파싱되어야 한다.
    assert(parsed.at("fetched") == 4);
    assert(parsed.at("failed") == 0);

    const fs::path cache = fs::path(parsed.at("cache_path").get<std::string>());
    assert(fs::exists(cache));

    iggen::TemplateStore loaded;
    assert(iggen::load_store(cache, loaded));
    assert(loaded.templates.count("python") == 1);
    assert(loaded.schema_version == iggen::kTemplateStoreSchemaVersion);

    fs::remove_all(data_dir);
}

void test_update_partial_failure_returns_4(MockApi &api) {
    assert_local_api();
    const auto data_dir = make_temp_dir("update_partial");
    set_data_dir(data_dir);
    api.list_body = "python,notatemplate";

    const auto r = run_cli({"update", "--json"});

    assert(r.code == 4);
    const auto parsed = nlohmann::json::parse(r.out);
    assert(parsed.at("status") == "partial");
    assert(parsed.at("fetched") == 1);
    assert(parsed.at("failed") == 1);

    fs::remove_all(data_dir);
}

void test_update_list_failure_is_network_error(MockApi &api) {
    assert_local_api();
    api.list_status = 503;
    const auto r = run_cli({"update"});
    api.list_status = 200;

    assert(r.code == 3);
    assert(r.err.find("Failed to fetch template list") != std::string::npos);
}

} // namespace

auto main() -> int {
    MockApi api;
    if (!api.start()) {
        std::cerr << "Failed to start the mock API server\n";
        return 1;
    }

    // 목 서버를 가리키고 재시도 지연을 없앤다. 사용자 캐시는 임시 디렉터리로 격리한다.
    set_env("IGGEN_API_BASE_URL", api.base_url());
    set_env("IGGEN_API_RETRIES", "0");
    const auto isolated_data = make_temp_dir("default");
    set_data_dir(isolated_data);

    // 진행 상황을 stderr로 남겨 실패/지연 시 어느 케이스인지 바로 알 수 있게 한다.
#define IGGEN_RUN(fn)                                                                              \
    do {                                                                                           \
        std::cerr << "[ run] " << #fn << std::endl;                                                \
        fn;                                                                                        \
    } while (0)

    IGGEN_RUN(test_version_flag());
    IGGEN_RUN(test_help_lists_docs_and_exit_codes());
    IGGEN_RUN(test_dry_run_keeps_stdout_clean(api));
    IGGEN_RUN(test_quiet_silences_stderr());
    IGGEN_RUN(test_json_dry_run_is_machine_readable());
    IGGEN_RUN(test_invalid_template_name_is_rejected_before_network(api));
    IGGEN_RUN(test_usage_errors_return_exit_code_2());
    IGGEN_RUN(test_existing_file_needs_confirmation_without_tty());
    IGGEN_RUN(test_legacy_dry_run_alias_warns());
    IGGEN_RUN(test_api_key_file_missing_is_usage_error());
    IGGEN_RUN(test_api_failure_falls_back_to_builtin(api));
    IGGEN_RUN(test_unknown_template_without_local_fallback_is_network_error(api));
    IGGEN_RUN(test_update_writes_cache_and_returns_zero(api));
    IGGEN_RUN(test_update_partial_failure_returns_4(api));
    IGGEN_RUN(test_update_list_failure_is_network_error(api));
#undef IGGEN_RUN

    fs::remove_all(isolated_data);
    std::cout << "All CLI tests passed.\n";
    return 0;
}

#pragma once

// Must be defined before including httplib.h (set in CMakeLists.txt).
#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
#define CPPHTTPLIB_OPENSSL_SUPPORT
#endif

#include <httplib.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

namespace iggen {

struct ApiResult {
    bool success;
    std::string body;
};

namespace detail {

// 일시적 오류(연결 실패, 429, 5xx)에 대한 재시도 횟수. 테스트에서는
// IGGEN_API_RETRIES=0 으로 지연을 없앨 수 있다.
constexpr int kDefaultRetries = 2;
constexpr int kTimeoutSeconds = 10;

inline auto retry_count() -> int {
    if (const char *env = std::getenv("IGGEN_API_RETRIES"); env && *env) {
        try {
            return std::max(0, std::min(5, std::stoi(env)));
        } catch (...) {
            return kDefaultRetries;
        }
    }
    return kDefaultRetries;
}

// 지수 백오프 + 지터. 재시도 폭주(Thundering herd)를 막기 위해 지터를 더한다.
inline void backoff_sleep(int attempt) {
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> jitter(0, 100);
    const auto delay = std::chrono::milliseconds(200 * (1 << attempt) + jitter(rng));
    std::this_thread::sleep_for(delay);
}

inline auto is_retryable_status(int status) -> bool {
    return status == 429 || status >= 500;
}

struct Endpoint {
    bool tls = true;
    std::string scheme_host_port; // 예: "www.toptal.com", "127.0.0.1:8123"
    std::string base_path;        // 예: "" 또는 "/developers/gitignore/api"
};

// 엔드포인트 재정의. 기본값은 gitignore.io(Toptal) HTTPS 엔드포인트이며,
// 사내 미러나 테스트용 목 서버를 위해 IGGEN_API_BASE_URL 로 바꿀 수 있다.
// http:// 를 지정하면 TLS가 꺼지므로 로컬 테스트 용도로만 사용한다.
inline auto parse_endpoint(const std::string &raw) -> Endpoint {
    Endpoint ep;
    std::string rest = raw;
    if (rest.rfind("http://", 0) == 0) {
        ep.tls = false;
        rest = rest.substr(7);
    } else if (rest.rfind("https://", 0) == 0) {
        ep.tls = true;
        rest = rest.substr(8);
    }
    const auto slash = rest.find('/');
    if (slash == std::string::npos) {
        ep.scheme_host_port = rest;
    } else {
        ep.scheme_host_port = rest.substr(0, slash);
        ep.base_path = rest.substr(slash);
    }
    while (!ep.base_path.empty() && ep.base_path.back() == '/') {
        ep.base_path.pop_back();
    }
    return ep;
}

inline auto api_base_url() -> std::string {
    if (const char *env = std::getenv("IGGEN_API_BASE_URL"); env && *env) {
        return env;
    }
    return "https://www.toptal.com";
}

inline auto api_endpoint() -> Endpoint {
    return parse_endpoint(api_base_url());
}

template <typename ClientT>
inline auto request_with_retries(ClientT &client, const std::string &path) -> ApiResult {
    const int retries = retry_count();
    ApiResult last{false, "connection failed"};
    for (int attempt = 0; attempt <= retries; ++attempt) {
        auto res = client.Get(path);
        if (!res) {
            last = {false, "connection failed: " + httplib::to_string(res.error())};
        } else if (res->status != 200) {
            last = {false, "HTTP " + std::to_string(res->status)};
            if (!is_retryable_status(res->status)) {
                return last;
            }
        } else if (res->body.empty()) {
            return {false, "empty response from API"};
        } else {
            return {true, res->body};
        }
        if (attempt < retries) {
            backoff_sleep(attempt);
        }
    }
    return last;
}

inline auto get(const std::string &path) -> ApiResult {
    const Endpoint ep = api_endpoint();
    const std::string full_path = ep.base_path + path;
    if (ep.tls) {
        httplib::SSLClient client(ep.scheme_host_port);
        client.set_connection_timeout(kTimeoutSeconds);
        client.set_read_timeout(kTimeoutSeconds);
        // 시스템 CA 저장소로 서버 인증서를 검증해 중간자 공격을 막는다.
        // httplib은 OpenSSL 기본 경로로 폴백한다.
        client.enable_server_certificate_verification(true);
        return request_with_retries(client, full_path);
    }
    httplib::Client client(ep.scheme_host_port);
    client.set_connection_timeout(kTimeoutSeconds);
    client.set_read_timeout(kTimeoutSeconds);
    return request_with_retries(client, full_path);
}

} // namespace detail

// 템플릿 이름으로 허용되는 문자만 통과시킨다. 이름은 그대로 URL 경로에 들어가므로
// 검증 없이 쓰면 경로 주입(SSRF/디렉터리 탈출)이 가능하다.
inline auto is_valid_template_name(std::string_view name) -> bool {
    if (name.empty() || name.size() > 64) {
        return false;
    }
    for (const unsigned char c : name) {
        const bool allowed =
            std::isalnum(c) != 0 || c == '+' || c == '-' || c == '_' || c == '.' || c == '#';
        if (!allowed) {
            return false;
        }
    }
    return true;
}

// Fetches the combined .gitignore content for a set of templates.
inline ApiResult fetch_gitignore(const std::set<std::string> &templates) {
    if (templates.empty()) {
        return {false, "no templates specified"};
    }
    std::ostringstream joined;
    bool first = true;
    for (const auto &t : templates) {
        if (!is_valid_template_name(t)) {
            return {false, "invalid template name: " + t};
        }
        if (!first)
            joined << ',';
        joined << t;
        first = false;
    }
    return detail::get("/developers/gitignore/api/" + joined.str());
}

// Fetches the comma-separated list of all available template names.
inline ApiResult fetch_template_list() {
    return detail::get("/developers/gitignore/api/list");
}

// Fetches the .gitignore content for a single template.
inline ApiResult fetch_template(const std::string &name) {
    if (name.empty()) {
        return {false, "empty template name"};
    }
    if (!is_valid_template_name(name)) {
        return {false, "invalid template name: " + name};
    }
    return detail::get("/developers/gitignore/api/" + name);
}

} // namespace iggen

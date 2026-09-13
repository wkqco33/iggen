#include "api_client.hpp"

#include <cassert>
#include <iostream>
#include <set>
#include <string>

// These tests exercise only the non-network code paths of api_client.hpp so
// they run offline. Live API calls are intentionally not covered here.
namespace {

void test_fetch_gitignore_empty_templates() {
    auto r = iggen::fetch_gitignore({});
    assert(!r.success);
    assert(r.body == "no templates specified");
}

void test_fetch_template_empty_name() {
    auto r = iggen::fetch_template("");
    assert(!r.success);
    assert(r.body == "empty template name");
}

void test_template_name_validation() {
    // gitignore.io에 실제로 존재하는 형태의 이름은 통과해야 한다.
    assert(iggen::is_valid_template_name("python"));
    assert(iggen::is_valid_template_name("c++"));
    assert(iggen::is_valid_template_name("visualstudiocode"));
    assert(iggen::is_valid_template_name("appcode+iml"));
    assert(iggen::is_valid_template_name("1c-bitrix"));
    assert(iggen::is_valid_template_name(std::string(64, 'a')));

    // 경로 주입/오타로 URL을 오염시킬 수 있는 입력은 거부한다.
    assert(!iggen::is_valid_template_name(""));
    assert(!iggen::is_valid_template_name("../etc"));
    assert(!iggen::is_valid_template_name("a/b"));
    assert(!iggen::is_valid_template_name("a b"));
    assert(!iggen::is_valid_template_name("python?x=1"));
    assert(!iggen::is_valid_template_name("python\nnode"));
    assert(!iggen::is_valid_template_name(std::string(65, 'a')));
}

void test_invalid_names_fail_without_network() {
    auto combined = iggen::fetch_gitignore({"../etc"});
    assert(!combined.success);
    assert(combined.body.find("invalid template name") != std::string::npos);

    auto single = iggen::fetch_template("a/b");
    assert(!single.success);
    assert(single.body.find("invalid template name") != std::string::npos);
}

void test_endpoint_parsing() {
    const auto https_ep = iggen::detail::parse_endpoint("https://www.toptal.com");
    assert(https_ep.tls);
    assert(https_ep.scheme_host_port == "www.toptal.com");
    assert(https_ep.base_path.empty());

    // http:// 는 TLS를 끄므로 로컬 목 서버 테스트에서만 쓴다.
    const auto local_ep = iggen::detail::parse_endpoint("http://127.0.0.1:8123/");
    assert(!local_ep.tls);
    assert(local_ep.scheme_host_port == "127.0.0.1:8123");
    assert(local_ep.base_path.empty());

    // 경로 접두사가 있으면 유지하고 끝의 '/'는 제거한다.
    const auto prefix_ep = iggen::detail::parse_endpoint("https://mirror.example.com/gitignore/");
    assert(prefix_ep.tls);
    assert(prefix_ep.scheme_host_port == "mirror.example.com");
    assert(prefix_ep.base_path == "/gitignore");

    const auto no_scheme_ep = iggen::detail::parse_endpoint("www.toptal.com");
    assert(no_scheme_ep.tls);
    assert(no_scheme_ep.scheme_host_port == "www.toptal.com");
}

} // namespace

auto main() -> int {
    test_fetch_gitignore_empty_templates();
    test_fetch_template_empty_name();
    test_template_name_validation();
    test_invalid_names_fail_without_network();
    test_endpoint_parsing();
    std::cout << "All api_client tests passed.\n";
    return 0;
}

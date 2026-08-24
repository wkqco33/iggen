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

} // namespace

auto main() -> int {
    test_fetch_gitignore_empty_templates();
    test_fetch_template_empty_name();
    std::cout << "All api_client tests passed.\n";
    return 0;
}

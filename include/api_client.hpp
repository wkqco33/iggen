#pragma once

// Must be defined before including httplib.h (set in CMakeLists.txt).
#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
#define CPPHTTPLIB_OPENSSL_SUPPORT
#endif

#include <httplib.h>
#include <set>
#include <sstream>
#include <string>

namespace iggen {

struct ApiResult {
    bool success;
    std::string body;
};

inline ApiResult fetch_gitignore(const std::set<std::string> &templates) {
    if (templates.empty()) {
        return {false, "no templates specified"};
    }

    std::ostringstream joined;
    bool first = true;
    for (const auto &t : templates) {
        if (!first)
            joined << ',';
        joined << t;
        first = false;
    }

    httplib::SSLClient client("www.toptal.com");
    client.set_connection_timeout(10);
    client.set_read_timeout(10);
    client.enable_server_certificate_verification(false);

    const std::string path = "/developers/gitignore/api/" + joined.str();
    auto res = client.Get(path);

    if (!res) {
        return {false, "connection failed: " + httplib::to_string(res.error())};
    }
    if (res->status != 200) {
        return {false, "HTTP " + std::to_string(res->status)};
    }
    if (res->body.empty()) {
        return {false, "empty response from API"};
    }
    return {true, res->body};
}

} // namespace iggen

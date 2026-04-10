#pragma once

// CPPHTTPLIB_OPENSSL_SUPPORT must be defined before including httplib.h.
// It is set as a compile definition in CMakeLists.txt; guard here for safety.
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

inline ApiResult fetch_gitignore(const std::set<std::string>& templates) {
    if (templates.empty()) {
        return {false, "no templates specified"};
    }

    // Build comma-separated template string
    std::ostringstream joined;
    bool first = true;
    for (const auto& t : templates) {
        if (!first) joined << ',';
        joined << t;
        first = false;
    }

    // SSL certificate verification is disabled for convenience in a personal utility.
    // To harden: remove the line below and set a CA bundle via set_ca_cert_path().
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

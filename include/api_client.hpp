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

namespace detail {

inline ApiResult get(const std::string &path) {
    httplib::SSLClient client("www.toptal.com");
    client.set_connection_timeout(10);
    client.set_read_timeout(10);
    // Verify the server certificate against the system CA store to prevent
    // man-in-the-middle attacks. httplib falls back to OpenSSL's default paths.
    client.enable_server_certificate_verification(true);
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

} // namespace detail

// Fetches the combined .gitignore content for a set of templates.
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
    return detail::get("/developers/gitignore/api/" + name);
}

} // namespace iggen

#pragma once

#include <algorithm>
#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>

#include "ignore_rules.hpp"

namespace iggen {

// Transparent hash so unordered_map<string_view,...> can be looked up by string_view.
struct StringViewHash {
    using is_transparent = void;
    size_t operator()(std::string_view sv) const noexcept {
        return std::hash<std::string_view>{}(sv);
    }
};

inline auto
extension_map() -> const std::unordered_map<std::string_view, std::string_view, StringViewHash> & {
    static const std::unordered_map<std::string_view, std::string_view, StringViewHash> map = {
        // C / C++
        {"cpp", "c++"},
        {"cc", "c++"},
        {"cxx", "c++"},
        {"hpp", "c++"},
        {"hxx", "c++"},
        {"c", "c"},
        {"h", "c"},
        // Rust
        {"rs", "rust"},
        // Go
        {"go", "go"},
        // Java / JVM
        {"java", "java"},
        {"gradle", "java"},
        {"kt", "kotlin"},
        {"kts", "kotlin"},
        {"scala", "scala"},
        {"sbt", "scala"},
        {"groovy", "java"},
        // Python
        {"py", "python"},
        {"pyw", "python"},
        // JavaScript / TypeScript
        {"js", "node"},
        {"mjs", "node"},
        {"cjs", "node"},
        {"ts", "node"},
        {"tsx", "react"},
        {"jsx", "react"},
        {"vue", "vuejs"},
        // C# / .NET
        {"cs", "csharp"},
        {"fs", "fsharp"},
        {"fsx", "fsharp"},
        // Swift
        {"swift", "swift"},
        // Ruby
        {"rb", "ruby"},
        // PHP
        {"php", "php"},
        // Dart
        {"dart", "dart"},
        // Lua
        {"lua", "lua"},
        // R
        {"r", "r"},
        {"rmd", "r"},
        // Julia
        {"jl", "julia"},
        // Elixir
        {"ex", "elixir"},
        {"exs", "elixir"},
        // Erlang
        {"erl", "erlang"},
        {"hrl", "erlang"},
        // Haskell
        {"hs", "haskell"},
        {"lhs", "haskell"},
        // Clojure
        {"clj", "clojure"},
        {"cljs", "clojure"},
        // CMake
        {"cmake", "cmake"},
        // Terraform
        {"tf", "terraform"},
        {"tfvars", "terraform"},
        // Shell
        {"sh", "linux"},
        {"bash", "linux"},
        {"zsh", "linux"},
        {"fish", "linux"},
        // PowerShell
        {"ps1", "windows"},
        {"psm1", "windows"},
    };
    return map;
}

// Returns the file extension (without leading '.') as a string_view, lowercased.
// Uses a reusable thread-local buffer to avoid per-call heap allocation.
inline std::string_view file_extension(const std::filesystem::path &p) {
    static thread_local std::string buf;
    buf.clear();
#ifdef _WIN32
    const auto ext = p.extension().string();
#else
    const auto ext_path = p.extension();
    const auto &ext = ext_path.native();
#endif
    const size_t start = (!ext.empty() && ext[0] == '.') ? 1 : 0;
    buf.reserve(ext.size() - start);
    for (size_t i = start; i < ext.size(); ++i) {
        buf.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ext[i]))));
    }
    return buf;
}

inline auto detect_languages(const std::filesystem::path &root) -> std::set<std::string> {
    namespace fs = std::filesystem;
    const auto &ext_map = extension_map();
    std::set<std::string> result;

    std::error_code init_err;
    for (fs::recursive_directory_iterator
             it(root, fs::directory_options::skip_permission_denied, init_err),
         end;
         it != end; ++it) {
        if (it->is_directory()) {
            const auto name = it->path().filename().string();
            // 숨김 디렉터리(.git, .venv ...)와 빌드/의존성 디렉터리는 순회하지 않는다.
            if (is_hidden_directory(name) || is_ignored_directory(name) ||
                is_cmake_build_directory(name)) {
                it.disable_recursion_pending();
            }
            continue;
        }
        if (!it->is_regular_file()) {
            continue;
        }

        const auto ext = file_extension(it->path());
        if (ext.empty()) {
            continue;
        }
        auto found = ext_map.find(ext);
        if (found != ext_map.end()) {
            result.insert(std::string(found->second));
        }
    }
    return result;
}

} // namespace iggen

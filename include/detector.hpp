#pragma once

#include <algorithm>
#include <filesystem>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace iggen {

inline auto extension_map() -> const std::unordered_map<std::string, std::string>& {
    static const std::unordered_map<std::string, std::string> map = {
        // C / C++
        {"cpp", "c++"}, {"cc", "c++"}, {"cxx", "c++"}, {"hpp", "c++"}, {"hxx", "c++"},
        {"c", "c"},     {"h", "c"},
        // Rust
        {"rs", "rust"},
        // Go
        {"go", "go"},
        // Java / JVM
        {"java", "java"}, {"gradle", "java"},
        {"kt", "kotlin"}, {"kts", "kotlin"},
        {"scala", "scala"}, {"sbt", "scala"},
        {"groovy", "java"},
        // Python
        {"py", "python"}, {"pyw", "python"},
        // JavaScript / TypeScript
        {"js", "node"}, {"mjs", "node"}, {"cjs", "node"}, {"ts", "node"},
        {"tsx", "react"}, {"jsx", "react"},
        {"vue", "vuejs"},
        // C# / .NET
        {"cs", "csharp"}, {"fs", "fsharp"}, {"fsx", "fsharp"},
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
        {"r", "r"}, {"rmd", "r"},
        // Julia
        {"jl", "julia"},
        // Elixir
        {"ex", "elixir"}, {"exs", "elixir"},
        // Erlang
        {"erl", "erlang"}, {"hrl", "erlang"},
        // Haskell
        {"hs", "haskell"}, {"lhs", "haskell"},
        // Clojure
        {"clj", "clojure"}, {"cljs", "clojure"},
        // CMake
        {"cmake", "cmake"},
        // Terraform
        {"tf", "terraform"}, {"tfvars", "terraform"},
        // Shell
        {"sh", "linux"}, {"bash", "linux"}, {"zsh", "linux"}, {"fish", "linux"},
        // PowerShell
        {"ps1", "windows"}, {"psm1", "windows"},
    };
    return map;
}

inline auto detect_languages(const std::filesystem::path& root) -> std::set<std::string> {
    static const std::unordered_set<std::string> skip_dirs = {
        ".git",  "build",         "out",      "node_modules", "__pycache__",
        "target","vendor",        "dist",     "wcppcli",
    };

    namespace fs = std::filesystem;
    const auto& ext_map = extension_map();
    std::set<std::string> result;

    std::error_code init_err;
    for (fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied,
                                             init_err),
         end;
         it != end; ++it) {
        if (it->is_directory()) {
            const auto name = it->path().filename().string();
            const bool is_hidden    = !name.empty() && name[0] == '.';
            const bool is_skipped   = skip_dirs.count(name) > 0;
            const bool is_cmake_bld = name.rfind("cmake-build-", 0) == 0;
            if (is_hidden || is_skipped || is_cmake_bld) {
                it.disable_recursion_pending();
            }
            continue;
        }
        if (!it->is_regular_file()) {
            continue;
        }

        auto ext = it->path().extension().string();
        if (ext.empty()) {
            continue;
        }
        ext = ext.substr(1); // strip leading '.'
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char chr) { return std::tolower(chr); });

        auto found = ext_map.find(ext);
        if (found != ext_map.end()) {
            result.insert(found->second);
        }
    }
    return result;
}

} // namespace iggen

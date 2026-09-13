#pragma once

#include <algorithm>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "ignore_rules.hpp"

namespace iggen {

struct ProjectContext {
    std::filesystem::path root_dir;
    std::vector<std::string> detected_build_files;
    std::vector<std::string> sample_file_tree;
    std::map<std::string, int> extension_counts;
};

inline auto is_known_build_file(const std::string &name) -> bool {
    static const std::set<std::string> known = {"CMakeLists.txt",
                                                "CMakePresets.json",
                                                "Makefile",
                                                "package.json",
                                                "package-lock.json",
                                                "yarn.lock",
                                                "pnpm-lock.yaml",
                                                "Cargo.toml",
                                                "Cargo.lock",
                                                "go.mod",
                                                "go.sum",
                                                "pom.xml",
                                                "build.gradle",
                                                "settings.gradle",
                                                "requirements.txt",
                                                "pyproject.toml",
                                                "Pipfile",
                                                "setup.py",
                                                "Gemfile",
                                                "composer.json",
                                                ".env",
                                                ".env.example",
                                                ".env.local"};
    return known.find(name) != known.end();
}

inline auto scan_project_context(const std::filesystem::path &root, int max_depth = 4,
                                 size_t max_tree_entries = 60) -> ProjectContext {
    ProjectContext ctx;
    ctx.root_dir = root;

    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::exists(root, ec) || !fs::is_directory(root, ec)) {
        return ctx;
    }

    auto iter =
        fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied, ec);
    auto end = fs::recursive_directory_iterator();

    while (iter != end && !ec) {
        const auto &entry = *iter;
        const auto depth = iter.depth();
        const auto filename = entry.path().filename().string();

        if (entry.is_directory(ec)) {
            if (is_ignored_directory(filename) || is_cmake_build_directory(filename) ||
                depth >= max_depth) {
                iter.disable_recursion_pending();
            }
            iter.increment(ec);
            continue;
        }

        if (entry.is_regular_file(ec)) {
            if (is_known_build_file(filename)) {
                ctx.detected_build_files.push_back(filename);
            }

            auto ext = entry.path().extension().string();
            if (!ext.empty()) {
                std::transform(ext.begin(), ext.end(), ext.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                ctx.extension_counts[ext]++;
            }

            if (ctx.sample_file_tree.size() < max_tree_entries) {
                auto rel = fs::relative(entry.path(), root, ec);
                if (!ec) {
                    ctx.sample_file_tree.push_back(rel.generic_string());
                }
            }
        }

        iter.increment(ec);
    }

    std::sort(ctx.detected_build_files.begin(), ctx.detected_build_files.end());
    ctx.detected_build_files.erase(
        std::unique(ctx.detected_build_files.begin(), ctx.detected_build_files.end()),
        ctx.detected_build_files.end());

    std::sort(ctx.sample_file_tree.begin(), ctx.sample_file_tree.end());

    return ctx;
}

} // namespace iggen

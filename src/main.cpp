#include <ctime>
#include <filesystem>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "wcppcli/wcli.hpp"
#include "wcppcli/wlog.hpp"
#include "wcppcli/wui.hpp"

#include "api_client.hpp"
#include "detector.hpp"
#include "file_writer.hpp"
#include "paths.hpp"
#include "template_store.hpp"

namespace fs = std::filesystem;
using namespace wcppcli;

static auto split_comma(const std::string &str) -> std::vector<std::string> {
    std::vector<std::string> result;
    std::istringstream stream(str);
    std::string token;
    while (std::getline(stream, token, ',')) {
        if (!token.empty()) {
            result.push_back(token);
        }
    }
    return result;
}

// Current date as YYYY-MM-DD (used as the cache version stamp).
static auto today_iso() -> std::string {
    std::time_t now = std::time(nullptr);
    std::tm tmv{};
#if defined(_WIN32)
    localtime_s(&tmv, &now);
#else
    localtime_r(&now, &tmv);
#endif
    char buf[16];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tmv);
    return buf;
}

// The effective store: user cache merged over the compiled-in defaults.
static auto effective_store() -> iggen::TemplateStore {
    iggen::TemplateStore user;
    if (iggen::load_store(iggen::default_cache_path(), user)) {
        return iggen::merge_stores(user, iggen::builtin_store());
    }
    return iggen::builtin_store();
}

struct ResolveResult {
    std::string content;
    bool used_fallback;
    std::vector<std::string> missing;
};

// Resolves .gitignore content for a template set. Tries the live API first and
// falls back to the local store (user cache -> built-in) when the API fails.
static auto resolve_content(const std::set<std::string> &templates) -> ResolveResult {
    auto api = iggen::fetch_gitignore(templates);
    if (api.success) {
        return {api.body, false, {}};
    }

    const auto &store = effective_store();
    std::vector<std::string> missing;
    for (const auto &t : templates) {
        if (!store.contains(t)) {
            missing.push_back(t);
        }
    }
    return {iggen::render_templates(store, templates), true, missing};
}

int main(int argc, char **argv) {
    Command root;
    root.name = "iggen";
    root.description = "Auto-generates .gitignore via gitignore.io API (offline fallback built-in)";
    root.usage = "iggen [--lang <langs>] [--no-defaults] [--output <file>] | iggen update";

    std::string langs_raw;
    bool no_defaults = false;
    std::string output = ".gitignore";

    Flag lang_flag;
    lang_flag.name = "lang";
    lang_flag.shorthand = 'l';
    lang_flag.description = "Comma-separated language list (overrides auto-detect)";
    lang_flag.value_ptr = &langs_raw;
    root.add_flag(lang_flag);

    Flag nodef_flag;
    nodef_flag.name = "no-defaults";
    nodef_flag.description = "Skip default templates (visualstudiocode, linux, macos, windows)";
    nodef_flag.value_ptr = &no_defaults;
    root.add_flag(nodef_flag);

    Flag out_flag;
    out_flag.name = "output";
    out_flag.shorthand = 'o';
    out_flag.description = "Output file path (default: .gitignore)";
    out_flag.value_ptr = &output;
    root.add_flag(out_flag);

    // `iggen update` — refresh the local template cache from gitignore.io.
    auto update_cmd = std::make_unique<Command>();
    update_cmd->name = "update";
    update_cmd->description = "Refresh the local template cache from gitignore.io";
    update_cmd->handler = [](const Command &) -> int {
        auto list = iggen::fetch_template_list();
        if (!list.success) {
            WLog::error("Failed to fetch template list: " + list.body);
            std::exit(1);
        }
        auto names = split_comma(list.body);
        if (names.empty()) {
            WLog::error("Empty template list from API");
            std::exit(1);
        }

        WLog::info("Fetching " + std::to_string(names.size()) + " templates...");
        iggen::TemplateStore store;
        store.version = today_iso();
        store.source = "gitignore.io";
        store.fetched_at = today_iso();

        int ok = 0;
        for (const auto &name : names) {
            auto r = iggen::fetch_template(name);
            if (r.success) {
                store.templates[name] = r.body;
                ++ok;
            } else {
                WLog::warn("Skipped " + name + ": " + r.body);
            }
        }
        if (ok == 0) {
            WLog::error("No templates could be fetched");
            std::exit(1);
        }

        const auto cache = iggen::default_cache_path();
        if (!iggen::save_store(cache, store)) {
            WLog::error("Failed to write cache: " + cache.string());
            std::exit(1);
        }
        WLog::success("Updated cache: " + std::to_string(ok) + " templates -> " + cache.string());
        return 0;
    };
    root.add_command(std::move(update_cmd));

    root.handler = [&](const Command &) -> int {
        // 1. Determine template set
        std::set<std::string> templates;

        if (!langs_raw.empty()) {
            for (const auto &lang : split_comma(langs_raw)) {
                templates.insert(lang);
            }
        } else {
            WLog::info("Scanning project files...");
            templates = iggen::detect_languages(fs::current_path());
            if (templates.empty()) {
                WLog::warn("No recognizable source files found; using default templates only.");
            }
        }

        // 2. Add default OS/editor templates unless suppressed
        if (!no_defaults) {
            for (const auto *tpl : {"linux", "macos", "visualstudiocode", "windows"}) {
                templates.insert(tpl);
            }
        }

        // 3. Show fetching info
        std::ostringstream list;
        bool first = true;
        for (const auto &tpl : templates) {
            if (!first) {
                list << ", ";
            }
            list << tpl;
            first = false;
        }
        WLog::info("Fetching .gitignore for: " + list.str());

        // 4. Resolve content (API first, then local store fallback)
        auto resolved = resolve_content(templates);
        if (resolved.used_fallback) {
            WLog::warn("gitignore.io unavailable; using local template store.");
            if (!resolved.missing.empty()) {
                std::ostringstream miss;
                bool mfirst = true;
                for (const auto &m : resolved.missing) {
                    if (!mfirst)
                        miss << ", ";
                    miss << m;
                    mfirst = false;
                }
                WLog::warn("No local template for: " + miss.str());
            }
        }
        if (resolved.content.empty()) {
            WLog::error("No .gitignore content could be produced.");
            std::exit(1);
        }

        // 5. Write output file
        const auto write_result = iggen::write_output(output, resolved.content);
        if (write_result == iggen::WriteResult::Error) {
            WLog::error("Failed to write: " + output);
            std::exit(1);
        }
        if (write_result == iggen::WriteResult::Skipped) {
            WLog::warn("Skipped. Use -o to specify a different output file.");
            return 0;
        }

        WLog::success("Generated: " + output);
        return 0;
    };

    return root.execute(argc, argv);
}

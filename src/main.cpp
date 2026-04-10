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

namespace fs = std::filesystem;
using namespace wcppcli;

static auto split_comma(const std::string& str) -> std::vector<std::string> {
    std::vector<std::string> result;
    std::istringstream       stream(str);
    std::string              token;
    while (std::getline(stream, token, ',')) {
        if (!token.empty()) {
            result.push_back(token);
        }
    }
    return result;
}

int main(int argc, char** argv) {
    Command root;
    root.name        = "iggen";
    root.description = "Auto-generates .gitignore via gitignore.io API";
    root.usage       = "iggen [--lang <langs>] [--no-defaults] [--output <file>]";

    std::string langs_raw;
    bool        no_defaults = false;
    std::string output      = ".gitignore";

    Flag lang_flag;
    lang_flag.name        = "lang";
    lang_flag.shorthand   = 'l';
    lang_flag.description = "Comma-separated language list (overrides auto-detect)";
    lang_flag.value_ptr   = &langs_raw;
    root.add_flag(lang_flag);

    Flag nodef_flag;
    nodef_flag.name        = "no-defaults";
    nodef_flag.description = "Skip default templates (visualstudiocode, linux, macos, windows)";
    nodef_flag.value_ptr   = &no_defaults;
    root.add_flag(nodef_flag);

    Flag out_flag;
    out_flag.name        = "output";
    out_flag.shorthand   = 'o';
    out_flag.description = "Output file path (default: .gitignore)";
    out_flag.value_ptr   = &output;
    root.add_flag(out_flag);

    root.handler = [&](const Command&) {
        // 1. Determine template set
        std::set<std::string> templates;

        if (!langs_raw.empty()) {
            for (const auto& lang : split_comma(langs_raw)) {
                templates.insert(lang);
            }
        } else {
            WLog::info("Scanning project files...");
            templates = iggen::detect_languages(fs::current_path());
            if (templates.empty()) {
                WLog::error(
                    "No recognizable source files found. Use -l/--lang to specify manually.");
                std::exit(1);
            }
        }

        // 2. Add default OS/editor templates unless suppressed
        if (!no_defaults) {
            for (const auto* tpl : {"linux", "macos", "visualstudiocode", "windows"}) {
                templates.insert(tpl);
            }
        }

        // 3. Show fetching info
        std::ostringstream list;
        bool               first = true;
        for (const auto& tpl : templates) {
            if (!first) {
                list << ", ";
            }
            list << tpl;
            first = false;
        }
        WLog::info("Fetching .gitignore for: " + list.str());

        // 4. Call gitignore.io API
        auto api_result = iggen::fetch_gitignore(templates);
        if (!api_result.success) {
            WLog::error("API request failed: " + api_result.body);
            std::exit(1);
        }

        // 5. Write output file
        const auto write_result = iggen::write_output(output, api_result.body);
        if (write_result == iggen::WriteResult::Error) {
            WLog::error("Failed to write: " + output);
            std::exit(1);
        }
        if (write_result == iggen::WriteResult::Skipped) {
            WLog::warn("Skipped. Use -o to specify a different output file.");
            return;
        }

        WLog::success("Generated: " + output);
    };

    return root.execute(argc, argv);
}

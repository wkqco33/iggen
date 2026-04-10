#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include "wcppcli/wui.hpp"

#ifdef _WIN32
#include <io.h>
inline auto is_stdin_tty() -> bool { return _isatty(0) != 0; }
#else
#include <unistd.h>
inline auto is_stdin_tty() -> bool { return isatty(STDIN_FILENO) != 0; }
#endif

namespace iggen {

enum class WriteResult { Written, Skipped, Error };

// path uses std::filesystem::path to distinguish it from the string content.
inline auto write_output(const std::filesystem::path& path, const std::string& content)
    -> WriteResult {
    if (std::filesystem::exists(path)) {
        if (!is_stdin_tty()) {
            // Non-interactive environment: refuse silent overwrite.
            return WriteResult::Skipped;
        }
        const bool overwrite =
            wcppcli::ui::confirm("'" + path.string() + "' already exists. Overwrite?", false);
        if (!overwrite) {
            return WriteResult::Skipped;
        }
    }

    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out) {
        return WriteResult::Error;
    }
    out << content;
    return WriteResult::Written;
}

} // namespace iggen

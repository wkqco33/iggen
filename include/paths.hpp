#pragma once

#include <cstdlib>
#include <filesystem>

namespace iggen {

// Returns the per-user data directory for iggen, creating nothing.
//   Linux:   $XDG_DATA_HOME/iggen  or  ~/.local/share/iggen
//   macOS:   ~/Library/Application Support/iggen
//   Windows: %APPDATA%/iggen
// Falls back to the system temp dir if no home is resolvable.
inline std::filesystem::path user_data_dir() {
#ifdef _WIN32
    if (const char *a = std::getenv("APPDATA"); a && *a) {
        return std::filesystem::path(a) / "iggen";
    }
    return std::filesystem::temp_directory_path() / "iggen";
#elif defined(__APPLE__)
    if (const char *h = std::getenv("HOME"); h && *h) {
        return std::filesystem::path(h) / "Library" / "Application Support" / "iggen";
    }
    return std::filesystem::temp_directory_path() / "iggen";
#else
    if (const char *x = std::getenv("XDG_DATA_HOME"); x && *x) {
        return std::filesystem::path(x) / "iggen";
    }
    if (const char *h = std::getenv("HOME"); h && *h) {
        return std::filesystem::path(h) / ".local" / "share" / "iggen";
    }
    return std::filesystem::temp_directory_path() / "iggen";
#endif
}

// Path to the user cache file holding the full template snapshot.
inline std::filesystem::path default_cache_path() {
    return user_data_dir() / "templates.json";
}

} // namespace iggen

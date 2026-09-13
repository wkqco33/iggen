#pragma once

#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>

#include <nlohmann/json.hpp>

#include "default_templates.hpp"

namespace iggen {

// 캐시 파일 스키마 버전. 필드를 추가할 때는 올리고, 로더는 낮은 버전도 계속 읽는다.
constexpr int kTemplateStoreSchemaVersion = 1;

// A snapshot of gitignore.io templates: template name -> .gitignore content.
struct TemplateStore {
    int schema_version = kTemplateStoreSchemaVersion;
    std::string version;
    std::string source;
    std::string fetched_at;
    std::map<std::string, std::string> templates;

    bool empty() const {
        return templates.empty();
    }
    bool contains(const std::string &name) const {
        return templates.count(name) > 0;
    }
    const std::string *find(const std::string &name) const {
        auto it = templates.find(name);
        return it == templates.end() ? nullptr : &it->second;
    }
};

// The templates compiled into the binary (offline fallback).
inline const TemplateStore &builtin_store() {
    static const TemplateStore store = [] {
        TemplateStore s;
        s.version = DEFAULT_TEMPLATES_VERSION;
        s.source = "builtin";
        s.templates = default_templates();
        return s;
    }();
    return store;
}

// Loads a store from a JSON file. Returns false on missing/invalid file.
inline bool load_store(const std::filesystem::path &path, TemplateStore &out) {
    std::ifstream in(path);
    if (!in) {
        return false;
    }
    nlohmann::json j;
    try {
        in >> j;
    } catch (...) {
        return false;
    }
    if (!j.contains("templates") || !j["templates"].is_object()) {
        return false;
    }
    out.schema_version = j.value("schema_version", kTemplateStoreSchemaVersion);
    out.version = j.value("version", "");
    out.source = j.value("source", "");
    out.fetched_at = j.value("fetched_at", "");
    out.templates.clear();
    for (auto &[key, value] : j["templates"].items()) {
        if (value.is_string()) {
            out.templates[key] = value.get<std::string>();
        }
    }
    return true;
}

// Writes a store to a JSON file, creating parent directories as needed.
// 임시 파일에 먼저 쓴 뒤 rename 하므로 중단/디스크 오류 시에도 캐시가 깨지지 않는다.
inline bool save_store(const std::filesystem::path &path, const TemplateStore &store) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    nlohmann::json j;
    j["schema_version"] = store.schema_version;
    j["version"] = store.version;
    j["source"] = store.source;
    j["fetched_at"] = store.fetched_at;
    j["templates"] = store.templates;

    const std::filesystem::path tmp = std::filesystem::path(path.string() + ".tmp");
    {
        std::ofstream out(tmp, std::ios::out | std::ios::trunc);
        if (!out) {
            return false;
        }
        out << j.dump(2);
        out.flush();
        if (!out) {
            out.close();
            std::filesystem::remove(tmp, ec);
            return false;
        }
    }

    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        // Windows에서는 대상이 존재하면 rename이 실패하므로 제거 후 재시도한다.
        std::error_code ignore;
        std::filesystem::remove(path, ignore);
        ec.clear();
        std::filesystem::rename(tmp, path, ec);
        if (ec) {
            std::filesystem::remove(tmp, ignore);
            return false;
        }
    }
    return true;
}

// Merges a user store over a builtin store. User entries win; builtin-only
// entries are preserved. Metadata (version/source/fetched_at) follows the user
// store when present.
inline TemplateStore merge_stores(const TemplateStore &user, const TemplateStore &builtin) {
    TemplateStore merged = builtin;
    for (const auto &[key, value] : user.templates) {
        merged.templates[key] = value;
    }
    if (!user.version.empty()) {
        merged.version = user.version;
    }
    if (!user.source.empty()) {
        merged.source = user.source;
    }
    if (!user.fetched_at.empty()) {
        merged.fetched_at = user.fetched_at;
    }
    return merged;
}

// Renders the combined .gitignore content for a set of template names from a
// store. Templates missing from the store are silently skipped.
inline std::string render_templates(const TemplateStore &store,
                                    const std::set<std::string> &names) {
    std::string out;
    for (const auto &name : names) {
        const std::string *content = store.find(name);
        if (!content) {
            continue;
        }
        out += *content;
        if (!out.empty() && out.back() != '\n') {
            out += '\n';
        }
    }
    return out;
}

} // namespace iggen

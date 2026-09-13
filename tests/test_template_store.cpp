#include "template_store.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>

namespace fs = std::filesystem;

namespace {

fs::path temp_json() {
    static int counter = 0;
    fs::path p = fs::temp_directory_path() / ("iggen_store_" + std::to_string(counter++));
    fs::remove(p);
    return p;
}

void test_builtin_store_has_detector_languages() {
    const auto &store = iggen::builtin_store();
    // Languages the detector can produce must be available offline.
    assert(store.contains("c++"));
    assert(store.contains("python"));
    assert(store.contains("node"));
    assert(store.contains("rust"));
    assert(store.contains("go"));
    assert(store.contains("visualstudiocode"));
    assert(store.contains("linux"));
    assert(store.contains("macos"));
    assert(store.contains("windows"));
    // php is not a valid gitignore.io template and must be absent.
    assert(!store.contains("php"));
    assert(!store.empty());
}

void test_save_load_roundtrip() {
    iggen::TemplateStore s;
    s.version = "2026-01-01";
    s.source = "test";
    s.fetched_at = "now";
    s.templates["python"] = "# python\n__pycache__/\n";
    s.templates["node"] = "# node\nnode_modules/\n";

    auto p = temp_json();
    assert(iggen::save_store(p, s));

    iggen::TemplateStore loaded;
    assert(iggen::load_store(p, loaded));
    assert(loaded.version == "2026-01-01");
    assert(loaded.source == "test");
    assert(loaded.fetched_at == "now");
    assert(loaded.templates.size() == 2);
    assert(loaded.templates["python"] == "# python\n__pycache__/\n");
    assert(loaded.templates["node"] == "# node\nnode_modules/\n");
    fs::remove(p);
}

void test_load_missing_file_fails() {
    iggen::TemplateStore s;
    assert(!iggen::load_store(fs::temp_directory_path() / "does_not_exist_iggen.json", s));
}

void test_merge_user_overrides_builtin() {
    const auto &builtin = iggen::builtin_store();
    iggen::TemplateStore user;
    user.version = "2026-02-01";
    user.source = "cache";
    user.templates["python"] = "# custom python\n";
    user.templates["newlang"] = "# new\n";

    auto merged = iggen::merge_stores(user, builtin);
    // User value wins for python.
    assert(merged.templates["python"] == "# custom python\n");
    // Builtin-only entries are preserved.
    assert(merged.contains("c++"));
    // New user entries are added.
    assert(merged.templates["newlang"] == "# new\n");
    // Version/source come from the user store.
    assert(merged.version == "2026-02-01");
    assert(merged.source == "cache");
}

void test_schema_version_and_atomic_save() {
    auto p = temp_json();
    iggen::TemplateStore s;
    s.templates["python"] = "x";
    assert(iggen::save_store(p, s));

    // 원자적 저장: 임시 파일이 남지 않는다.
    assert(!fs::exists(fs::path(p.string() + ".tmp")));

    iggen::TemplateStore loaded;
    assert(iggen::load_store(p, loaded));
    assert(loaded.schema_version == iggen::kTemplateStoreSchemaVersion);

    // schema_version이 없는 예전 캐시도 계속 읽을 수 있어야 한다(하위 호환).
    {
        std::ofstream out(p, std::ios::trunc);
        out << R"({"version":"old","templates":{"python":"y"}})";
    }
    iggen::TemplateStore legacy;
    assert(iggen::load_store(p, legacy));
    assert(legacy.schema_version == iggen::kTemplateStoreSchemaVersion);
    assert(legacy.templates["python"] == "y");

    fs::remove(p);
}

void test_save_store_replaces_existing_file() {
    auto p = temp_json();
    {
        std::ofstream out(p);
        out << "not json at all";
    }

    iggen::TemplateStore s;
    s.templates["node"] = "n";
    assert(iggen::save_store(p, s));

    iggen::TemplateStore loaded;
    assert(iggen::load_store(p, loaded));
    assert(loaded.templates["node"] == "n");
    fs::remove(p);
}

void test_render_templates() {
    iggen::TemplateStore s;
    s.templates["a"] = "AAA\n";
    s.templates["b"] = "BBB";
    std::set<std::string> names = {"a", "b"};
    auto out = iggen::render_templates(s, names);
    assert(out.find("AAA") != std::string::npos);
    assert(out.find("BBB") != std::string::npos);
    // Missing templates are skipped without error.
    names.insert("missing");
    auto out2 = iggen::render_templates(s, names);
    assert(out2 == out);
}

} // namespace

auto main() -> int {
    test_builtin_store_has_detector_languages();
    test_save_load_roundtrip();
    test_load_missing_file_fails();
    test_merge_user_overrides_builtin();
    test_schema_version_and_atomic_save();
    test_save_store_replaces_existing_file();
    test_render_templates();
    std::cout << "All template_store tests passed.\n";
    return 0;
}

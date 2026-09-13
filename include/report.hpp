#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace iggen {

// `--json` 출력 계약. 필드 이름은 기계 판독용 계약으로 취급하며,
// 하위 호환을 깨는 변경(이름 변경·제거·타입 변경)은 금지하고 추가만 한다.
struct RunReport {
    std::string status; // "ok" | "error"
    bool dry_run = false;
    std::string output;
    std::vector<std::string> templates;
    std::string source; // "api" | "local"
    std::vector<std::string> missing;
    bool ai_requested = false;
    bool ai_applied = false;
    std::string content; // dry-run일 때만 채운다
    std::string error;

    // update 커맨드에서만 채워지는 필드.
    int fetched = 0;
    int failed = 0;
    std::string cache_path;
};

inline auto to_json(const RunReport &report) -> std::string {
    nlohmann::json j;
    j["status"] = report.status;
    j["dry_run"] = report.dry_run;
    if (!report.output.empty()) {
        j["output"] = report.output;
    }
    j["templates"] = report.templates;
    if (!report.source.empty()) {
        j["source"] = report.source;
    }
    if (!report.missing.empty()) {
        j["missing"] = report.missing;
    }
    j["ai_requested"] = report.ai_requested;
    j["ai_applied"] = report.ai_applied;
    if (!report.content.empty()) {
        j["content"] = report.content;
    }
    if (!report.error.empty()) {
        j["error"] = report.error;
    }
    if (report.fetched > 0 || report.failed > 0) {
        j["fetched"] = report.fetched;
        j["failed"] = report.failed;
    }
    if (!report.cache_path.empty()) {
        j["cache_path"] = report.cache_path;
    }
    return j.dump(2);
}

} // namespace iggen

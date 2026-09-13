#pragma once

#include "wcppcli/wui.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace iggen {

enum class WriteResult {
    Written,
    Skipped,           // 사용자가 대화형 확인에서 거부함 (오류 아님)
    NeedsConfirmation, // 비대화형이라 확인할 수 없음: -y/--yes 또는 -o 안내 필요
    Error,
    DryRun,
};

// `.gitignore` 내용을 파일로 쓰거나(dry_run=false) stdout으로 출력한다(dry_run=true).
// 기존 파일이 있으면 대화형일 때만 확인 프롬프트를 띄우고, 비대화형에서는 조용히
// 덮어쓰지 않고 NeedsConfirmation을 반환한다(호출자가 0이 아닌 코드로 처리).
inline auto write_output(const std::filesystem::path &path, const std::string &content,
                         bool dry_run = false, std::ostream &out_stream = std::cout,
                         bool force_overwrite = false) -> WriteResult {
    if (dry_run) {
        out_stream << content;
        return WriteResult::DryRun;
    }

    if (std::filesystem::exists(path) && !force_overwrite) {
        if (!wcppcli::ui::interactive_enabled()) {
            return WriteResult::NeedsConfirmation;
        }
        if (!wcppcli::ui::confirm("'" + path.string() + "' already exists. Overwrite?", false)) {
            return WriteResult::Skipped;
        }
    }

    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out) {
        return WriteResult::Error;
    }
    out << content;
    out.flush();
    if (!out) {
        return WriteResult::Error;
    }
    return WriteResult::Written;
}

} // namespace iggen

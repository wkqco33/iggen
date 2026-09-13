#pragma once

#include <algorithm>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "wcppcli/wcli.hpp"
#include "wcppcli/wlog.hpp"
#include "wcppcli/wui.hpp"

#include "ai_refiner.hpp"
#include "api_client.hpp"
#include "detector.hpp"
#include "exit_code.hpp"
#include "file_writer.hpp"
#include "iggen_version.hpp"
#include "paths.hpp"
#include "project_scanner.hpp"
#include "report.hpp"
#include "secret_input.hpp"
#include "template_store.hpp"

namespace iggen {
namespace cli_detail {

using wcppcli::Command;
using wcppcli::Flag;
using wcppcli::LogLevel;
using wcppcli::WLog;

constexpr const char *kDefaultTemplates[] = {"linux", "macos", "visualstudiocode", "windows"};

struct Options {
    // 생성 옵션
    std::string langs_raw;
    bool no_defaults = false;
    std::string output = ".gitignore";
    bool dry_run = false;
    bool assume_yes = false;
    // 전역 옵션
    bool quiet = false;
    bool debug = false;
    bool no_color = false;
    bool no_input = false;
    bool json = false;
    // AI 옵션
    bool use_ai = false;
    std::string ai_provider = "ollama";
    std::string ai_model = "llama3";
    std::string ai_base_url = "http://localhost:11434";
    std::string ai_api_key; // 해석된 최종 값(파일/stdin/플래그/env 중 하나)
    std::string ai_api_key_file;
    bool ai_api_key_stdin = false;
    std::string ai_api_key_flag; // deprecated
};

inline auto trim(std::string_view text) -> std::string {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(" \t\r\n");
    return std::string(text.substr(first, last - first + 1));
}

// 쉼표 또는 개행으로 구분된 목록을 파싱한다. gitignore.io의 /list 응답은
// 줄바꿈으로 감싸서 내려오므로 개행도 구분자로 취급해야 한다.
inline auto split_list(const std::string &text) -> std::vector<std::string> {
    std::vector<std::string> result;
    std::string token;
    for (const char ch : text) {
        if (ch == ',' || ch == '\n' || ch == '\r') {
            auto value = trim(token);
            if (!value.empty()) {
                result.push_back(value);
            }
            token.clear();
        } else {
            token.push_back(ch);
        }
    }
    auto value = trim(token);
    if (!value.empty()) {
        result.push_back(value);
    }
    return result;
}

inline auto join(const std::vector<std::string> &items, const std::string &sep) -> std::string {
    std::string out;
    for (size_t i = 0; i < items.size(); ++i) {
        if (i > 0) {
            out += sep;
        }
        out += items[i];
    }
    return out;
}

inline auto join(const std::set<std::string> &items, const std::string &sep) -> std::string {
    std::vector<std::string> values(items.begin(), items.end());
    return join(values, sep);
}

// Current date as YYYY-MM-DD (used as the cache version stamp).
inline auto today_iso() -> std::string {
    const std::time_t now = std::time(nullptr);
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
inline auto effective_store() -> TemplateStore {
    TemplateStore user;
    if (load_store(default_cache_path(), user)) {
        return merge_stores(user, builtin_store());
    }
    return builtin_store();
}

struct ResolveResult {
    std::string content;
    bool used_fallback = false;
    std::string source; // "api" | "local"
    std::vector<std::string> missing;
    std::string api_error;
};

// Resolves .gitignore content for a template set. Tries the live API first and
// falls back to the local store (user cache -> built-in) when the API fails.
inline auto resolve_content(const std::set<std::string> &templates) -> ResolveResult {
    const auto api = fetch_gitignore(templates);
    if (api.success) {
        return {api.body, false, "api", {}, {}};
    }

    const auto &store = effective_store();
    std::vector<std::string> missing;
    for (const auto &t : templates) {
        if (!store.contains(t)) {
            missing.push_back(t);
        }
    }
    return {render_templates(store, templates), true, "local", missing, api.body};
}

// 플래그 파싱 후 적용되는 공통 동작(로그 레벨·색상·프롬프트 정책).
inline void apply_common_options(const Options &opts) {
    if (opts.no_color) {
        wcppcli::set_color_enabled(false);
    }
    if (opts.no_input) {
        wcppcli::ui::set_interactive_enabled(false);
    }
    if (opts.quiet) {
        WLog::set_min_level(LogLevel::Error);
    } else if (opts.debug) {
        WLog::set_min_level(LogLevel::Debug);
    }
}

// `--json`이 켜져 있으면 결과를 stdout에 JSON 한 덩어리로 출력하고 종료 코드를 돌려준다.
inline auto finish(const Options &opts, RunReport report, ExitCode code) -> int {
    if (opts.json) {
        report.status = (code == ExitCode::Ok)               ? "ok"
                        : (code == ExitCode::PartialFailure) ? "partial"
                                                             : "error";
        std::cout << to_json(report) << std::endl;
    }
    return to_int(code);
}

inline auto fail(const Options &opts, RunReport report, const std::string &message,
                 ExitCode code) -> int {
    WLog::error(message);
    report.error = message;
    return finish(opts, report, code);
}

// 모든 커맨드에서 쓸 수 있는 전역 플래그.
inline void add_common_flags(Command &cmd, Options &opts) {
    Flag quiet;
    quiet.name = "quiet";
    quiet.shorthand = 'q';
    quiet.description = "Suppress status/progress messages on stderr";
    quiet.value_ptr = &opts.quiet;
    cmd.add_flag(quiet);

    Flag debug;
    debug.name = "debug";
    debug.description = "Enable debug logging on stderr";
    debug.value_ptr = &opts.debug;
    cmd.add_flag(debug);

    Flag no_color;
    no_color.name = "no-color";
    no_color.description = "Disable colored output (same as the NO_COLOR environment variable)";
    no_color.value_ptr = &opts.no_color;
    cmd.add_flag(no_color);

    Flag no_input;
    no_input.name = "no-input";
    no_input.description = "Never prompt; fail with exit code 2 when input would be required";
    no_input.value_ptr = &opts.no_input;
    cmd.add_flag(no_input);

    Flag json;
    json.name = "json";
    json.description = "Print a machine-readable JSON result to stdout";
    json.value_ptr = &opts.json;
    cmd.add_flag(json);
}

inline void add_generate_flags(Command &cmd, Options &opts) {
    Flag lang;
    lang.name = "lang";
    lang.shorthand = 'l';
    lang.description = "Comma-separated language list (overrides auto-detect)";
    lang.value_ptr = &opts.langs_raw;
    cmd.add_flag(lang);

    Flag no_defaults;
    no_defaults.name = "no-defaults";
    no_defaults.description = "Skip default templates (linux, macos, visualstudiocode, windows)";
    no_defaults.value_ptr = &opts.no_defaults;
    cmd.add_flag(no_defaults);

    Flag output;
    output.name = "output";
    output.shorthand = 'o';
    output.description = "Output file path (default: .gitignore)";
    output.value_ptr = &opts.output;
    cmd.add_flag(output);

    Flag dry_run;
    dry_run.name = "dry-run";
    dry_run.shorthand = 'n';
    dry_run.description = "Print the generated .gitignore to stdout without writing a file";
    dry_run.value_ptr = &opts.dry_run;
    cmd.add_flag(dry_run);

    Flag dry_run_legacy;
    dry_run_legacy.name = "dry-run-legacy";
    dry_run_legacy.shorthand = 'd';
    dry_run_legacy.description = "(deprecated) alias of -n/--dry-run";
    dry_run_legacy.value_ptr = &opts.dry_run;
    cmd.add_flag(dry_run_legacy);

    Flag yes;
    yes.name = "yes";
    yes.shorthand = 'y';
    yes.description = "Assume yes: overwrite an existing output file without prompting";
    yes.value_ptr = &opts.assume_yes;
    cmd.add_flag(yes);

    Flag use_ai;
    use_ai.name = "ai";
    use_ai.shorthand = 'a';
    use_ai.description = "Refine .gitignore for this project using an LLM";
    use_ai.value_ptr = &opts.use_ai;
    cmd.add_flag(use_ai);

    Flag ai_provider;
    ai_provider.name = "ai-provider";
    ai_provider.description = "LLM provider (default: ollama)";
    ai_provider.value_ptr = &opts.ai_provider;
    cmd.add_flag(ai_provider);

    Flag ai_model;
    ai_model.name = "ai-model";
    ai_model.description = "LLM model (default: llama3)";
    ai_model.value_ptr = &opts.ai_model;
    cmd.add_flag(ai_model);

    Flag ai_base_url;
    ai_base_url.name = "ai-base-url";
    ai_base_url.description = "LLM base endpoint URL (default: http://localhost:11434)";
    ai_base_url.value_ptr = &opts.ai_base_url;
    cmd.add_flag(ai_base_url);

    Flag ai_api_key;
    ai_api_key.name = "ai-api-key";
    ai_api_key.description = "(deprecated) API key; use --ai-api-key-file or --ai-api-key-stdin";
    ai_api_key.value_ptr = &opts.ai_api_key_flag;
    cmd.add_flag(ai_api_key);

    Flag ai_api_key_file;
    ai_api_key_file.name = "ai-api-key-file";
    ai_api_key_file.description = "Read the LLM API key from a file (first line)";
    ai_api_key_file.value_ptr = &opts.ai_api_key_file;
    cmd.add_flag(ai_api_key_file);

    Flag ai_api_key_stdin;
    ai_api_key_stdin.name = "ai-api-key-stdin";
    ai_api_key_stdin.description =
        "Read the LLM API key from stdin (for pipes: `pass show key | iggen --ai-api-key-stdin`)";
    ai_api_key_stdin.value_ptr = &opts.ai_api_key_stdin;
    cmd.add_flag(ai_api_key_stdin);
}

inline void load_env_defaults(Options &opts) {
    if (const char *env = std::getenv("IGGEN_AI_PROVIDER")) {
        opts.ai_provider = env;
    }
    if (const char *env = std::getenv("IGGEN_AI_MODEL")) {
        opts.ai_model = env;
    }
    if (const char *env = std::getenv("IGGEN_AI_BASE_URL")) {
        opts.ai_base_url = env;
    } else if (const char *env = std::getenv("OLLAMA_HOST")) {
        opts.ai_base_url = env;
    }
    if (const char *env = std::getenv("IGGEN_AI_API_KEY")) {
        opts.ai_api_key = env;
    } else if (const char *env = std::getenv("OPENAI_API_KEY")) {
        opts.ai_api_key = env;
    }
}

// --ai-api-key-file / --ai-api-key-stdin / (deprecated) --ai-api-key / env 순으로 해석한다.
inline auto resolve_api_key(const Command &cmd, Options &opts, RunReport &report,
                            ExitCode &code) -> bool {
    if (cmd.flag_was_set("ai-api-key")) {
        WLog::warn("--ai-api-key is deprecated: the value leaks into the process list and shell "
                   "history. Use --ai-api-key-file or --ai-api-key-stdin.");
    }
    if (!opts.ai_api_key_file.empty()) {
        std::string key;
        if (!read_secret_file(opts.ai_api_key_file, key)) {
            report.error = "could not read API key file: " + opts.ai_api_key_file;
            WLog::error(report.error);
            code = ExitCode::UsageError;
            return false;
        }
        opts.ai_api_key = key;
        return true;
    }
    if (opts.ai_api_key_stdin) {
        std::string key;
        if (!read_secret_stdin(key)) {
            report.error = "no API key on stdin";
            WLog::error(report.error);
            code = ExitCode::UsageError;
            return false;
        }
        opts.ai_api_key = key;
    }
    return true;
}

inline auto run_generate(const Command &cmd, Options &opts) -> int {
    apply_common_options(opts);

    RunReport report;
    report.dry_run = opts.dry_run;
    report.output = opts.output;
    report.ai_requested = opts.use_ai;

    if (!cmd.args.empty()) {
        return fail(opts, report,
                    "unexpected argument: " + join(cmd.args, " ") + " (see 'iggen --help')",
                    ExitCode::UsageError);
    }
    if (cmd.flag_was_set("dry-run-legacy")) {
        WLog::warn("-d is deprecated; use -n/--dry-run.");
    }

    ExitCode code = ExitCode::Ok;
    if (!resolve_api_key(cmd, opts, report, code)) {
        return finish(opts, report, code);
    }

    // 1. Determine template set
    std::set<std::string> templates;
    if (!opts.langs_raw.empty()) {
        for (const auto &lang : split_list(opts.langs_raw)) {
            templates.insert(lang);
        }
    } else {
        WLog::info("Scanning project files...");
        templates = detect_languages(std::filesystem::current_path());
        if (templates.empty()) {
            WLog::warn("No recognizable source files found; using default templates only.");
        }
    }

    // 2. Add default OS/editor templates unless suppressed
    if (!opts.no_defaults) {
        for (const auto *tpl : kDefaultTemplates) {
            templates.insert(tpl);
        }
    }

    // 3. Reject invalid template names before building an API URL from them.
    for (const auto &tpl : templates) {
        if (!is_valid_template_name(tpl)) {
            return fail(opts, report, "invalid template name: '" + tpl + "'", ExitCode::UsageError);
        }
    }
    report.templates.assign(templates.begin(), templates.end());

    // 4. Resolve content (API first, then local store fallback)
    WLog::info("Fetching .gitignore for: " + join(templates, ", "));
    const auto resolved = resolve_content(templates);
    report.source = resolved.source;
    report.missing = resolved.missing;
    if (resolved.used_fallback) {
        WLog::warn("gitignore.io unavailable; using local template store.");
        if (!resolved.missing.empty()) {
            WLog::warn("No local template for: " + join(resolved.missing, ", "));
        }
    }
    if (resolved.content.empty()) {
        std::string message = "No .gitignore content could be produced.";
        if (!resolved.api_error.empty()) {
            message += " (API: " + resolved.api_error + ")";
        }
        return fail(opts, report, message,
                    resolved.used_fallback ? ExitCode::NetworkError : ExitCode::Error);
    }

    std::string final_content = resolved.content;

    // 5. If --ai is requested, refine using LLM
    if (opts.use_ai) {
        WLog::info("Scanning project files for AI context...");
        const auto proj_ctx = scan_project_context(std::filesystem::current_path());

        WLog::info("Refining .gitignore with AI (" + opts.ai_provider + " / " + opts.ai_model +
                   ")...");
        AiConfig ai_config;
        ai_config.provider = opts.ai_provider;
        ai_config.model = opts.ai_model;
        ai_config.base_url = opts.ai_base_url;
        ai_config.api_key = opts.ai_api_key;

        const auto ai_result = refine_gitignore(resolved.content, proj_ctx, ai_config);
        if (ai_result.success && !ai_result.content.empty()) {
            final_content = ai_result.content;
            report.ai_applied = true;
            WLog::success("Successfully refined .gitignore with AI.");
            if (!ai_result.summary.empty()) {
                WLog::info("AI 작업 요약:\n" + ai_result.summary);
            }
        } else {
            WLog::warn("AI refinement failed: " + ai_result.error_message);
            WLog::warn("Falling back to base gitignore template.");
        }
    }

    // 6. Write output file (or stdout if dry-run). With --json the raw content never
    //    touches stdout so the JSON document stays parseable.
    std::ostringstream json_sink;
    std::ostream &content_stream =
        opts.json ? static_cast<std::ostream &>(json_sink) : static_cast<std::ostream &>(std::cout);
    const auto write_result =
        write_output(opts.output, final_content, opts.dry_run, content_stream, opts.assume_yes);
    if (opts.json && opts.dry_run) {
        report.content = final_content;
    }

    switch (write_result) {
    case iggen::WriteResult::DryRun:
        return finish(opts, report, ExitCode::Ok);
    case iggen::WriteResult::Written:
        WLog::success("Generated: " + opts.output);
        return finish(opts, report, ExitCode::Ok);
    case iggen::WriteResult::Skipped:
        WLog::warn("Skipped: the existing file was kept.");
        return finish(opts, report, ExitCode::Ok);
    case iggen::WriteResult::NeedsConfirmation:
        return fail(opts, report,
                    "'" + opts.output +
                        "' already exists and stdin is not interactive. "
                        "Re-run with -y/--yes to overwrite, or pick another path with -o.",
                    ExitCode::UsageError);
    case iggen::WriteResult::Error:
    default:
        return fail(opts, report, "Failed to write: " + opts.output, ExitCode::Error);
    }
}

inline auto run_update(const Command &cmd, Options &opts) -> int {
    apply_common_options(opts);
    RunReport report;

    if (!cmd.args.empty()) {
        return fail(opts, report,
                    "unexpected argument: " + join(cmd.args, " ") + " (see 'iggen update --help')",
                    ExitCode::UsageError);
    }

    const auto list = fetch_template_list();
    if (!list.success) {
        return fail(opts, report, "Failed to fetch template list: " + list.body,
                    ExitCode::NetworkError);
    }
    const auto names = split_list(list.body);
    if (names.empty()) {
        return fail(opts, report, "Empty template list from API", ExitCode::NetworkError);
    }

    WLog::info("Fetching " + std::to_string(names.size()) + " templates...");
    TemplateStore store;
    store.version = today_iso();
    store.source = "gitignore.io";
    store.fetched_at = today_iso();

    int ok = 0;
    int failed = 0;
    for (size_t i = 0; i < names.size(); ++i) {
        const auto &name = names[i];
        if (!is_valid_template_name(name)) {
            ++failed;
            WLog::warn("Skipped invalid template name: " + name);
            continue;
        }
        const auto result = fetch_template(name);
        if (result.success) {
            store.templates[name] = result.body;
            ++ok;
        } else {
            ++failed;
            // gitignore.io 목록에는 개별 엔드포인트가 없는 템플릿이 섞여 있어 404가 난다.
            WLog::warn("Skipped " + name + ": " + result.body);
        }
        if ((i + 1) % 25 == 0) {
            WLog::info("... " + std::to_string(i + 1) + "/" + std::to_string(names.size()));
        }
    }
    if (ok == 0) {
        return fail(opts, report, "No templates could be fetched", ExitCode::NetworkError);
    }

    const auto cache = default_cache_path();
    if (!save_store(cache, store)) {
        return fail(opts, report, "Failed to write cache: " + cache.string(), ExitCode::Error);
    }
    WLog::success("Updated cache: " + std::to_string(ok) + " templates -> " + cache.string());
    if (failed > 0) {
        WLog::warn(std::to_string(failed) +
                   " templates were skipped; the cache was saved with the rest.");
    }

    report.fetched = ok;
    report.failed = failed;
    report.cache_path = cache.string();
    return finish(opts, report, failed > 0 ? ExitCode::PartialFailure : ExitCode::Ok);
}

} // namespace cli_detail

// CLI 진입점. 프로세스 상태를 바꾸지 않고 종료 코드를 반환하므로
// 테스트에서 그대로 호출해 stdout/stderr/종료 코드를 검증할 수 있다.
inline auto run(int argc, char **argv) -> int {
    namespace cli = cli_detail;

    // 이전 실행(테스트)에서 바뀐 전역 상태를 초기화한다.
    wcppcli::WLog::reset_min_level();
    wcppcli::reset_color_enabled();
    wcppcli::ui::reset_interactive_enabled();

    cli::Options opts;
    cli::load_env_defaults(opts);

    wcppcli::Command root;
    root.name = "iggen";
    root.description =
        "Auto-generates .gitignore via gitignore.io API (offline fallback built-in, AI "
        "customization)";
    root.usage = "iggen [--lang <langs>] [--no-defaults] [--output <file>] [--ai] [--dry-run] | "
                 "iggen update";
    root.version = std::string("iggen ") + IGGEN_VERSION;
    root.usage_error_code = to_int(ExitCode::UsageError);
    root.epilog = "Documentation: https://github.com/wkqco33/iggen#readme\n"
                  "Issues:        https://github.com/wkqco33/iggen/issues\n"
                  "Exit codes:    0 ok, 1 error, 2 usage/input, 3 network, 4 partial failure\n"
                  "Examples:\n"
                  "  iggen --dry-run                     # preview without writing\n"
                  "  iggen -l python,node --no-defaults  # explicit languages only\n"
                  "  printf '%s' \"$API_KEY\" | iggen --ai --ai-api-key-stdin\n"
                  "  iggen update                        # refresh the local template cache";

    cli::add_common_flags(root, opts);
    cli::add_generate_flags(root, opts);

    auto update_cmd = std::make_unique<wcppcli::Command>();
    update_cmd->name = "update";
    update_cmd->description = "Refresh the local template cache from gitignore.io";
    update_cmd->usage = "iggen update [--json] [--quiet] [--debug] [--no-color] [--no-input]";
    update_cmd->epilog = "Documentation: https://github.com/wkqco33/iggen#readme\n"
                         "Example:       iggen update --quiet";
    cli::add_common_flags(*update_cmd, opts);
    update_cmd->handler = [&opts](const wcppcli::Command &cmd) {
        return cli::run_update(cmd, opts);
    };
    root.add_command(std::move(update_cmd));

    root.handler = [&opts](const wcppcli::Command &cmd) { return cli::run_generate(cmd, opts); };

    return root.execute(argc, argv);
}

} // namespace iggen

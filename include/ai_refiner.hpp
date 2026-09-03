#pragma once

#include "llm_client/exceptions.hpp"
#include "llm_client/llm_client_factory.hpp"
#include "llm_client/llm_client_interface.hpp"
#include "project_scanner.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace iggen {

struct AiConfig {
    std::string provider = "ollama";
    std::string model = "llama3";
    std::string base_url = "http://localhost:11434";
    std::string api_key;
    int timeout_ms = 45000;
};

struct AiRefineResult {
    bool success = false;
    std::string content;
    std::string summary;
    std::string error_message;
};

inline auto build_refine_prompt(const std::string &base_template,
                                const ProjectContext &ctx) -> std::string {
    std::ostringstream ss;
    ss << "Here is the project information scanned from the repository:\n\n";

    if (!ctx.detected_build_files.empty()) {
        ss << "Build & Configuration Files:\n";
        for (const auto &file : ctx.detected_build_files) {
            ss << "- " << file << "\n";
        }
        ss << "\n";
    }

    if (!ctx.extension_counts.empty()) {
        ss << "File Extension Counts:\n";
        for (const auto &[ext, count] : ctx.extension_counts) {
            ss << "- " << ext << ": " << count << "\n";
        }
        ss << "\n";
    }

    if (!ctx.sample_file_tree.empty()) {
        ss << "Repository Sample File Tree:\n";
        for (const auto &path : ctx.sample_file_tree) {
            ss << "- " << path << "\n";
        }
        ss << "\n";
    }

    ss << "Base .gitignore Template (from gitignore.io):\n"
       << "```gitignore\n"
       << base_template << "\n"
       << "```\n\n"
       << "Task:\n"
       << "1. 프로젝트 컨텍스트를 분석하여 어떤 조정을 수행했는지 간결하게 한국어(Korean)로 "
          "요약하세요 (2-4개 불릿 포인트).\n"
       << "2. 프로젝트에 최적화된 완전한 .gitignore 파일을 작성하세요.\n\n"
       << "Please format your response strictly as follows:\n"
       << "### SUMMARY\n"
       << "- <한국어 조정 요약 1>\n"
       << "- <한국어 조정 요약 2>\n\n"
       << "### GITIGNORE\n"
       << "```gitignore\n"
       << "<rules here>\n"
       << "```\n";

    return ss.str();
}

inline auto clean_llm_gitignore_output(const std::string &raw_output) -> std::string {
    auto start_pos = raw_output.find("```");
    if (start_pos == std::string::npos) {
        return raw_output;
    }

    // Skip the opening backticks and any optional language identifier on that line
    auto newline_pos = raw_output.find('\n', start_pos);
    if (newline_pos == std::string::npos) {
        return raw_output;
    }
    size_t code_start = newline_pos + 1;

    auto end_pos = raw_output.find("```", code_start);
    if (end_pos == std::string::npos) {
        return raw_output.substr(code_start);
    }

    return raw_output.substr(code_start, end_pos - code_start);
}

inline auto
parse_llm_refine_response(const std::string &raw_output) -> std::pair<std::string, std::string> {
    std::string summary;
    std::string gitignore;

    const std::string sum_tag = "### SUMMARY";
    const std::string git_tag = "### GITIGNORE";

    auto summary_pos = raw_output.find(sum_tag);
    auto gitignore_pos = raw_output.find(git_tag);

    if (summary_pos != std::string::npos && gitignore_pos != std::string::npos &&
        gitignore_pos > summary_pos) {
        size_t sum_start = summary_pos + sum_tag.length();
        summary = raw_output.substr(sum_start, gitignore_pos - sum_start);

        // Trim leading and trailing whitespace
        size_t first = summary.find_first_not_of(" \t\r\n");
        size_t last = summary.find_last_not_of(" \t\r\n");
        if (first != std::string::npos && last != std::string::npos) {
            summary = summary.substr(first, last - first + 1);
        } else {
            summary.clear();
        }

        std::string raw_git = raw_output.substr(gitignore_pos + git_tag.length());
        gitignore = clean_llm_gitignore_output(raw_git);
    } else {
        // Fallback: If tags are missing, treat the entire output as gitignore content
        gitignore = clean_llm_gitignore_output(raw_output);
    }

    return {summary, gitignore};
}

inline auto refine_gitignore_with_client(llm_client::LLMClientInterface &client,
                                         const std::string &base_template,
                                         const ProjectContext &ctx,
                                         const AiConfig &config) -> AiRefineResult {
    AiRefineResult result;

    std::string prompt = build_refine_prompt(base_template, ctx);

    std::vector<llm_client::Message> messages = {
        {"system",
         "You are an expert developer and Git configuration assistant. Your job is to analyze "
         "the project context, explain your adjustments under ### SUMMARY in Korean (한국어로 "
         "작성), "
         "and output an optimized, tailored .gitignore file under ### GITIGNORE."},
        {"user", prompt}};

    llm_client::RequestParams params;
    params.model = config.model;
    params.temperature = 0.2f;
    params.timeout_ms = config.timeout_ms;

    try {
        auto response = client.chat(messages, params);
        if (response.content.empty()) {
            result.success = false;
            result.error_message = "LLM returned empty response content";
            return result;
        }

        auto [summary, gitignore] = parse_llm_refine_response(response.content);
        result.summary = summary;
        result.content = gitignore;
        result.success = true;
    } catch (const std::exception &ex) {
        result.success = false;
        result.error_message = ex.what();
    }

    return result;
}

inline auto refine_gitignore(const std::string &base_template, const ProjectContext &ctx,
                             const AiConfig &config) -> AiRefineResult {
    try {
        auto client =
            llm_client::LLMClientFactory::create(config.provider, config.api_key, config.base_url);
        if (!client) {
            AiRefineResult res;
            res.success = false;
            res.error_message = "Unsupported LLM provider: " + config.provider;
            return res;
        }
        return refine_gitignore_with_client(*client, base_template, ctx, config);
    } catch (const std::exception &ex) {
        AiRefineResult res;
        res.success = false;
        res.error_message = ex.what();
        return res;
    }
}

} // namespace iggen

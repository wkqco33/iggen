#include "ai_refiner.hpp"
#include "llm_client/mock_http_client.hpp"
#include "llm_client/ollama_client.hpp"
#include "project_scanner.hpp"

#include <cassert>
#include <iostream>
#include <string>

namespace {

void test_build_prompt() {
    iggen::ProjectContext ctx;
    ctx.detected_build_files = {"CMakeLists.txt", ".env"};
    ctx.sample_file_tree = {"src/main.cpp", "include/main.hpp"};
    ctx.extension_counts[".cpp"] = 1;
    ctx.extension_counts[".hpp"] = 1;

    std::string base_template = "# Base template\nbuild/\n";
    auto prompt = iggen::build_refine_prompt(base_template, ctx);

    assert(prompt.find("CMakeLists.txt") != std::string::npos);
    assert(prompt.find(".env") != std::string::npos);
    assert(prompt.find("src/main.cpp") != std::string::npos);
    assert(prompt.find("# Base template") != std::string::npos);
}

void test_clean_llm_output() {
    // Case 1: Markdown codeblock with gitignore tag
    std::string input1 =
        "Here is the customized file:\n```gitignore\n# custom\n*.log\n```\nHope this helps!";
    std::string cleaned1 = iggen::clean_llm_gitignore_output(input1);
    assert(cleaned1.find("```") == std::string::npos);
    assert(cleaned1.find("Hope this helps") == std::string::npos);
    assert(cleaned1.find("*.log") != std::string::npos);

    // Case 2: Markdown codeblock without tag
    std::string input2 = "```\n*.tmp\n*.bak\n```";
    std::string cleaned2 = iggen::clean_llm_gitignore_output(input2);
    assert(cleaned2 == "*.tmp\n*.bak\n");

    // Case 3: Plain text without code blocks
    std::string input3 = "# Comment\n*.obj\n";
    std::string cleaned3 = iggen::clean_llm_gitignore_output(input3);
    assert(cleaned3 == "# Comment\n*.obj\n");
}

void test_mock_llm_refine_success() {
    auto mock_http = std::make_shared<llm_client::MockHttpClient>();
    std::string mock_json = R"({
        "message": {
            "role": "assistant",
            "content": "```gitignore\n# Refined by AI\n*.log\nbuild/\n.env\n```"
        },
        "done_reason": "stop"
    })";
    mock_http->setResponse(200, mock_json);

    auto client =
        std::make_unique<llm_client::OllamaClient>("http://localhost:11434", "", mock_http);

    iggen::ProjectContext ctx;
    ctx.detected_build_files = {"CMakeLists.txt"};
    std::string base_template = "build/\n";

    iggen::AiConfig config;
    config.model = "llama3";

    auto result = iggen::refine_gitignore_with_client(*client, base_template, ctx, config);
    assert(result.success);
    assert(result.error_message.empty());
    assert(result.content.find("# Refined by AI") != std::string::npos);
    assert(result.content.find(".env") != std::string::npos);
    assert(result.content.find("```") == std::string::npos);
}

void test_mock_llm_refine_failure_graceful() {
    auto mock_http = std::make_shared<llm_client::MockHttpClient>();
    mock_http->setError(7, "Failed to connect to localhost port 11434");

    auto client =
        std::make_unique<llm_client::OllamaClient>("http://localhost:11434", "", mock_http);

    iggen::ProjectContext ctx;
    std::string base_template = "build/\n";

    iggen::AiConfig config;

    auto result = iggen::refine_gitignore_with_client(*client, base_template, ctx, config);
    assert(!result.success);
    assert(!result.error_message.empty());
    assert(result.content.empty());
}

void test_parse_llm_response_with_summary() {
    std::string response = R"(### SUMMARY
- Added CMake build artifacts
- Added .env ignore rule

### GITIGNORE
```gitignore
# CMake
build/
.env
```
)";
    auto [summary, gitignore] = iggen::parse_llm_refine_response(response);
    assert(summary.find("Added CMake build artifacts") != std::string::npos);
    assert(summary.find("Added .env ignore rule") != std::string::npos);
    assert(gitignore.find("build/") != std::string::npos);
    assert(gitignore.find(".env") != std::string::npos);
    assert(gitignore.find("```") == std::string::npos);
}

void test_mock_llm_refine_with_summary() {
    auto mock_http = std::make_shared<llm_client::MockHttpClient>();
    std::string mock_json = R"({
        "message": {
            "role": "assistant",
            "content": "### SUMMARY\n- Tailored for C++ and CMake\n\n### GITIGNORE\n```gitignore\nbuild/\n*.o\n```"
        },
        "done_reason": "stop"
    })";
    mock_http->setResponse(200, mock_json);

    auto client =
        std::make_unique<llm_client::OllamaClient>("http://localhost:11434", "", mock_http);

    iggen::ProjectContext ctx;
    std::string base_template = "build/\n";

    iggen::AiConfig config;
    config.model = "llama3";

    auto result = iggen::refine_gitignore_with_client(*client, base_template, ctx, config);
    assert(result.success);
    assert(result.summary.find("Tailored for C++ and CMake") != std::string::npos);
    assert(result.content.find("build/") != std::string::npos);
}

} // namespace

auto main() -> int {
    test_build_prompt();
    test_clean_llm_output();
    test_parse_llm_response_with_summary();
    test_mock_llm_refine_success();
    test_mock_llm_refine_with_summary();
    test_mock_llm_refine_failure_graceful();
    std::cout << "All ai_refiner tests passed.\n";
    return 0;
}

#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace iggen {

// 시크릿(API 키)은 명령줄 플래그로 받지 않는다. 플래그는 프로세스 목록과 셸 히스토리에
// 남기 때문에 파일/표준입력 경로를 제공한다.

// 파일에서 시크릿을 읽는다. 후행 개행(CR/LF)은 제거하며, 비었으면 false.
inline auto read_secret_file(const std::filesystem::path &path, std::string &out) -> bool {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }
    std::string value((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    while (!value.empty() && (value.back() == '\n' || value.back() == '\r')) {
        value.pop_back();
    }
    if (value.empty()) {
        return false;
    }
    out = value;
    return true;
}

// 표준 입력에서 시크릿을 읽는다(파이프 전용). 후행 개행은 제거하며, 비었으면 false.
inline auto read_secret_stdin(std::string &out) -> bool {
    std::string value((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());
    while (!value.empty() && (value.back() == '\n' || value.back() == '\r')) {
        value.pop_back();
    }
    if (value.empty()) {
        return false;
    }
    out = value;
    return true;
}

} // namespace iggen

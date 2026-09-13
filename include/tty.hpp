#pragma once

// 표준 입력이 대화형 터미널인지 판별한다. 프롬프트를 노출해도 되는지 결정할 때 쓴다.
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace iggen {

inline auto stdin_is_tty() -> bool {
#ifdef _WIN32
    return _isatty(_fileno(stdin)) != 0;
#else
    return isatty(STDIN_FILENO) != 0;
#endif
}

} // namespace iggen

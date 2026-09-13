#include "cli.hpp"

// 진입점은 종료 코드 전달만 담당한다. 실제 CLI 로직은 include/cli.hpp에 있어
// 테스트에서 iggen::run()을 직접 호출할 수 있다.
auto main(int argc, char **argv) -> int {
    return iggen::run(argc, argv);
}

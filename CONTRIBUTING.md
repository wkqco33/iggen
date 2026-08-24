# Contributing

`iggen`에 기여해 주셔서 감사합니다! 아래 가이드를 따라주세요.

## 개발 환경

- C++17, CMake 3.20+, Ninja
- vcpkg (`cpp-httplib`, `OpenSSL`)
- `wcppcli` 서브모듈 (초기화: `git submodule update --init --recursive`)

## 작업 절차

1. 이슈를 확인하거나 새 이슈를 만듭니다.
2. 브랜치를 만들고 작업합니다.
3. **TDD** 방식으로 진행합니다: 먼저 실패하는 테스트를 작성하고, 구현 후 통과시킵니다.
4. 코드 스타일을 맞춥니다:

   ```bash
   clang-format -i src/*.cpp include/*.hpp tests/*.cpp
   clang-tidy src/*.cpp -- -std=c++17
   ```

5. 모든 테스트가 통과하는지 확인합니다:

   ```bash
   cmake --preset debug
   cmake --build --preset debug
   ctest --preset debug --output-on-failure
   ```

6. Pull Request를 열고 변경 사항을 설명합니다.

## 커밋 메시지

간결하고 행위 중심으로 작성합니다 (예: `Add detect_languages test`, `Fix timeout handling`).

## 라이선스

이 프로젝트는 [MIT](LICENSE) 라이선스로 배포됩니다. 기여 시 동일 라이선스에 동의하는 것으로 간주합니다.

## 행동 강령

모든 기여자는 [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md)를 준수해야 합니다.

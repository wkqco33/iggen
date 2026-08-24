# AGENTS.md — 개발 가이드

이 문서는 이 저장소에서 작업하는 모든 에이전트(및 개발자)가 따라야 할 규칙입니다.
코드 변경 전에 반드시 읽고 준수하세요.

## 프로젝트 개요

`iggen`은 현재 디렉토리를 스캔해 사용 중인 언어를 감지하고
[gitignore.io API](https://www.toptal.com/developers/gitignore/api/)로 `.gitignore`를 생성하는 C++17 CLI 도구입니다.

- 빌드 시스템: CMake + Ninja (프리셋: `debug`, `release`, `*-native`)
- 의존성: vcpkg (`cpp-httplib`, `OpenSSL`), `wcppcli` 서브모듈
- 언어: C++17, 헤더-온리 모듈 (`include/*.hpp`)

## 개발 워크플로우 (TDD)

이 프로젝트는 **TDD(테스트 주도 개발)** 방식으로 진행합니다. 순서를 지키세요:

1. **테스트 먼저 작성** — `tests/` 아래에 실패하는 테스트를 추가합니다.
2. **테스트 실행** — 실패를 확인합니다.
3. **최소 구현** — 테스트를 통과시키는 최소한의 코드를 작성합니다.
4. **리팩터링** — 중복 제거, 가독성 개선 후 테스트가 여전히 통과하는지 확인합니다.

### 테스트 실행

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

모든 변경은 `ctest`가 통과해야 합니다. 테스트가 없는 새 기능은 추가하지 마세요.

### 테스트 구조

- `tests/test_detector.cpp` — 언어 감지 로직
- `tests/test_file_writer.cpp` — 파일 출력 로직
- `tests/smoke_test.cpp` — 헤더 공용 컴파일/링크 스모크 테스트

테스트는 `assert` 기반의 단순 실행 파일로, 네트워크에 의존하지 않아야 합니다.
네트워크가 필요한 로직은 인터페이스를 분리해 목(mock)으로 테스트할 수 있게 하세요.

## 코드 스타일

- 포맷터: `clang-format` (`.clang-format`, LLVM 기반, 4-space, 100 col)
- 린터: `clang-tidy` (`.clang-tidy`)
- 변경 후 반드시 포맷을 맞추세요:

```bash
clang-format -i src/*.cpp include/*.hpp tests/*.cpp
clang-tidy src/*.cpp -- -std=c++17
```

## 커밋 규칙

- 커밋 메시지는 간결하고 행위 중심으로 작성합니다 (예: `Add detect_languages test`).
- 빌드 산출물, `vcpkg_installed/`, `build/` 등은 커밋하지 않습니다 (`.gitignore` 참고).
- 서브모듈(`wcppcli`)의 내용은 직접 수정하지 않습니다.

## 주의사항

- `include/*.hpp`는 헤더-온리(inline) 구현입니다. 새 모듈도 동일한 패턴을 따르세요.
- API 호출부(`api_client.hpp`)는 SSL 인증서 검증을 비활성화한 상태입니다. 보안 강화 시
  `enable_server_certificate_verification(true)` + CA 번들 설정을 사용하세요.
- `ppm.json`은 [PACKAGE_GUIDE.md](https://raw.githubusercontent.com/wkqco33/package_manager/refs/heads/master/PACKAGE_GUIDE.md) 규칙을 따릅니다.

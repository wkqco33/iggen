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
- `tests/test_template_store.cpp` — 템플릿 저장소(load/save/merge/render) 로직
- `tests/test_paths.cpp` — 사용자 데이터 디렉토리 해석
- `tests/test_api_client.cpp` — API 클라이언트의 비네트워크 경로(빈 입력 검증)
- `tests/smoke_test.cpp` — 헤더 공용 컴파일/링크 스모크 테스트

테스트는 `assert` 기반의 단순 실행 파일로, **네트워크에 의존하지 않아야 합니다**.
네트워크가 필요한 로직은 인터페이스를 분리해 목(mock)으로 테스트할 수 있게 하세요.
`api_client.hpp`는 실제 HTTP 호출 대신 빈 입력 등 비네트워크 경로만 테스트합니다.

### 테스트 추가 규칙

- 새 기능은 반드시 `tests/`에 실패하는 테스트를 먼저 추가하세요.
- 테스트는 `tests/CMakeLists.txt`에 `add_executable` + `add_test`로 등록하세요.
- 네트워크/파일시스템에 의존하는 테스트는 임시 디렉토리(`fs::temp_directory_path()`)를
  사용하고 정리하세요.

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

## 오프라인 폴백 설계

- `include/default_templates.hpp`는 `tools/fetch_defaults.py`로 생성되는 **자동 생성 파일**입니다.
  직접 수정하지 말고 스크립트로 재생성하세요.
- 생성 순서: **API → 사용자 캐시(`$XDG_DATA_HOME/iggen/templates.json`) → 내장 기본값**.
- `iggen update`는 gitignore.io에서 전체 템플릿을 가져와 사용자 캐시를 최신화합니다.
- gitignore.io 목록에는 개별 엔드포인트가 없는 템플릿(예: `php`)이 있어 404가 발생할 수 있습니다.
  이 경우 해당 템플릿은 건너뛰고 경고만 출력합니다.

## 성능 고려사항

- `detector.hpp`의 `file_extension()`은 핫 경로(파일마다 호출)이므로 재사용 가능한
  thread-local 버퍼를 사용해 힙 할당을 피합니다. POSIX에서는 `path::native()`를,
  Windows에서는 `path::string()`을 사용합니다.
- `extension_map()`은 투명 해시(`StringViewHash`)를 사용해 `string_view`로 조회합니다.
- `builtin_store()`는 정적 초기화로 한 번만 구성됩니다. 반복 호출 시 재구성하지 마세요.
- `effective_store()`는 API 실패 시에만 호출되므로 파일 I/O 비용이 정상 경로에 없습니다.

## 주의사항

- `include/*.hpp`는 헤더-온리(inline) 구현입니다. 새 모듈도 동일한 패턴을 따르세요.
- API 호출부(`api_client.hpp`)는 서버 인증서를 시스템 CA 저장소로 검증합니다
  (`enable_server_certificate_verification(true)`). 검증을 비활성화하지 마세요.
- `ppm.json`은 [PACKAGE_GUIDE.md](https://raw.githubusercontent.com/wkqco33/package_manager/refs/heads/master/PACKAGE_GUIDE.md) 규칙을 따릅니다.

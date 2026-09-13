# AGENTS.md — 개발 가이드

이 문서는 이 저장소에서 작업하는 모든 에이전트(및 개발자)가 따라야 하는 규칙입니다.
코드 변경 전에 반드시 읽고 준수하세요.

## 프로젝트 개요

`iggen`은 현재 디렉토리를 스캔해 사용 중인 언어를 감지하고
[gitignore.io API](https://www.toptal.com/developers/gitignore/api/)로 `.gitignore`를 생성하는 C++17 CLI 도구입니다.

- 빌드 시스템: CMake + Ninja (프리셋: `debug`, `release`, `*-native`)
- 의존성(vcpkg): `cpp-httplib`(openssl), `OpenSSL`, `nlohmann-json`, `cpr`, `spdlog`
- 의존성(FetchContent, 커밋 SHA 고정): `wcppcli`, `LLM_client`
  - 로컬 체크아웃으로 개발할 때: `-DFETCHCONTENT_SOURCE_DIR_WCPPCLI=/path/to/wcppcli`
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

CI와 동일한 조건(경고 = 오류)으로도 확인하세요:

```bash
cmake --preset debug -B build/strict -DIGGEN_WARNINGS_AS_ERRORS=ON
cmake --build build/strict
```

### 테스트 구조

- `tests/test_cli.cpp` — CLI 계약(종료 코드, stdout/stderr 분리, `--json`, 캐시 갱신) E2E
- `tests/test_detector.cpp` — 언어 감지 로직
- `tests/test_ignore_rules.cpp` — 스캐너 공용 제외 규칙
- `tests/test_file_writer.cpp` — 파일 출력/덮어쓰기 정책
- `tests/test_template_store.cpp` — 템플릿 저장소(load/save/merge/render)
- `tests/test_paths.cpp` — 사용자 데이터 디렉토리 해석
- `tests/test_api_client.cpp` — API 클라이언트의 비네트워크 경로(입력 검증, 엔드포인트 파싱)
- `tests/test_project_scanner.cpp` — AI 컨텍스트 스캔
- `tests/test_report.cpp` — `--json` 계약과 시크릿 입력
- `tests/test_ai_refiner.cpp` — LLM 응답 파싱
- `tests/smoke_test.cpp` — 헤더 공용 컴파일/링크 스모크 테스트
- `tools/generate_sbom.py --selftest` — 릴리스 SBOM 생성기 자체 테스트(ctest에 등록)

### 테스트 규칙

- **테스트는 외부 네트워크에 의존하지 않습니다.** HTTP가 필요하면 `httplib`으로
  `127.0.0.1` 목 서버를 띄우고 `IGGEN_API_BASE_URL`로 연결하세요(`test_cli.cpp` 참고).
  테스트 시작 시 `assert_local_api()`로 실수로 실제 API를 호출하지 않았는지 확인합니다.
- 파일시스템을 쓰는 테스트는 `fs::temp_directory_path()`를 사용하고 정리하세요.
- 환경변수를 바꾸면 테스트 끝에서 복구하세요(특히 `IGGEN_API_BASE_URL`).
- 새 기능은 반드시 실패하는 테스트를 먼저 추가하고, `tests/CMakeLists.txt`의
  `iggen_add_test()`로 등록하세요.

## CLI 계약 (변경 시 문서·테스트 동시 수정)

`include/cli.hpp`의 `iggen::run()`이 단일 진입점이며, `src/main.cpp`는 호출만 합니다.

- **종료 코드**(`include/exit_code.hpp`): `0` 성공, `1` 일반 오류, `2` 사용법/입력 오류,
  `3` 네트워크 오류, `4` 부분 실패. 새 실패 경로는 매직 넘버 대신 `ExitCode`를 사용하세요.
- **스트림**: 결과는 stdout, 진행/경고/오류는 stderr(`WLog`). 결과를 stdout에 섞지 마세요.
- **프롬프트**: 비TTY·`--no-input`에서는 프롬프트를 띄우지 말고 필요한 플래그를 안내한 뒤
  `2`로 실패합니다. 파괴적 작업은 `-y/--yes`로 비대화형 실행이 가능해야 합니다.
- **시크릿**: 명령줄 플래그로 받지 마세요. 파일(`--ai-api-key-file`)이나 stdin
  (`--ai-api-key-stdin`)을 사용합니다.
- 플래그/서브커맨드 변경은 additive하게 유지하고, 제거할 때는 deprecation 경고를 먼저 넣습니다.
- 종료 코드·플래그·환경변수를 바꾸면 `README.md`(종료 코드 표, 입출력 계약)와
  `CHANGELOG.md`를 같은 커밋에서 갱신하세요.

## 코드 스타일

- 포맷터: `clang-format` (`.clang-format`, LLVM 기반, 4-space, 100 col)
- 린터: `clang-tidy` (`.clang-tidy`)

```bash
clang-format -i src/*.cpp include/*.hpp tests/*.cpp
clang-tidy -p build/debug src/main.cpp
```

- 주석은 **왜**가 필요한 경우에만 간결하게 씁니다. 단계 번호, 자기 설명, 에이전트 독백,
  변경 이력 서술은 넣지 마세요.
- 헤더-온리 모듈이므로 `include/*.hpp`의 함수는 `inline`으로 유지합니다.

## 버전과 릴리스

- 버전의 단일 소스는 `CMakeLists.txt`의 `project(iggen VERSION x.y.z)`이며, CMake가
  `iggen_version.hpp`를 생성해 `--version`에 사용합니다.
- 버전을 올릴 때는 **`CMakeLists.txt`, `vcpkg.json`, `CHANGELOG.md`, git 태그**를 모두
  일치시키세요. 릴리스 워크플로우가 불일치 시 실패합니다.
- 릴리스 노트는 [Keep a Changelog](https://keepachangelog.com/) 형식(Added/Changed/Fixed/
  Security)을 따릅니다.
- 릴리스는 CI에서만 수행하며 SHA-256, SPDX SBOM, 빌드 provenance를 첨부합니다.
  SBOM은 `tools/generate_sbom.py`가 vcpkg `status`에서 의존성 목록을 만들어 생성하며,
  바이너리만 스캔하는 방식(의존성이 드러나지 않음)으로 되돌리지 마세요.
- GitHub Actions는 이동하는 태그(`@v4`)가 아니라 **커밋 SHA로 고정**합니다.

## 공급망/재현성

- vcpkg 의존성은 `vcpkg.json`의 `builtin-baseline`으로 고정합니다.
- `FetchContent` 의존성은 브랜치가 아니라 커밋 SHA로 고정합니다. 올릴 때는 해당 저장소를
  먼저 릴리스·push하고 전체 테스트를 통과시킨 뒤 SHA를 갱신하세요.

## 주의사항

- `include/default_templates.hpp`는 `tools/fetch_defaults.py`로 생성되는 **자동 생성 파일**입니다.
  직접 수정하지 말고 스크립트로 재생성하세요.
- 생성 순서: **API → 사용자 캐시(`$XDG_DATA_HOME/iggen/templates.json`) → 내장 기본값**.
- API 호출부(`api_client.hpp`)는 HTTPS에서 서버 인증서를 시스템 CA 저장소로 검증합니다
  (`enable_server_certificate_verification(true)`). 검증을 비활성화하지 마세요.
  `http://`는 `IGGEN_API_BASE_URL`을 통한 로컬 테스트에서만 허용됩니다.
- gitignore.io 목록에는 개별 엔드포인트가 없는 템플릿(예: `php`)이 있어 404가 발생할 수
  있습니다. 이 경우 해당 템플릿은 건너뛰고 경고만 출력합니다(`update`는 종료 코드 4).
- `ppm.json`은 [PACKAGE_GUIDE.md](https://raw.githubusercontent.com/wkqco33/package_manager/refs/heads/master/PACKAGE_GUIDE.md) 규칙을 따릅니다.

## 성능 고려사항

- `detector.hpp`의 `file_extension()`은 핫 경로(파일마다 호출)이므로 재사용 가능한
  thread-local 버퍼를 사용해 힙 할당을 피합니다.
- `extension_map()`은 투명 해시(`StringViewHash`)를 사용해 `string_view`로 조회합니다.
- `builtin_store()`는 정적 초기화로 한 번만 구성됩니다. 반복 호출 시 재구성하지 마세요.
- `ignore_rules.hpp`의 제외 목록은 이진 탐색을 사용하므로 **정렬을 유지**해야 합니다
  (`tests/test_ignore_rules.cpp`가 검증).
- `effective_store()`는 API 실패 시에만 호출되므로 파일 I/O 비용이 정상 경로에 없습니다.

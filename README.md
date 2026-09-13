# iggen

`.gitignore` 파일을 자동으로 생성해주는 CLI 도구.

현재 디렉토리의 파일을 재귀적으로 스캔하여 사용 중인 언어를 자동 감지하고,
[gitignore.io API](https://www.toptal.com/developers/gitignore/api/)를 통해 적합한 `.gitignore`를 생성합니다.

## 설치

```bash
ppm install wkqco33/iggen
```

> [ppm](https://github.com/seoyc/ppm)이 설치되어 있어야 합니다.

## 사용법

```bash
iggen [options]        # 현재 디렉토리를 스캔해 .gitignore 생성
iggen update [options] # gitignore.io 템플릿 캐시 최신화
```

### 생성 옵션

| 옵션                 | 설명                                                      |
| -------------------- | --------------------------------------------------------- |
| `-l, --lang <langs>` | 쉼표로 구분된 언어 목록 지정 (자동 감지 대신 사용)        |
| `--no-defaults`      | 기본 템플릿(visualstudiocode, linux, macos, windows) 제외 |
| `-o, --output <file>`| 출력 파일 경로 지정 (기본값: `.gitignore`)                |
| `-n, --dry-run`      | 파일에 쓰지 않고 생성된 `.gitignore`를 stdout으로 출력    |
| `-y, --yes`          | 기존 출력 파일을 확인 없이 덮어쓰기                       |
| `-a, --ai`           | LLM을 활용하여 프로젝트에 맞춤 정제된 `.gitignore` 생성   |

> `-d`는 `--dry-run`의 deprecated 별칭입니다. 사용 시 경고가 출력되며 `-n`을 쓰세요.

### 공통 옵션

| 옵션            | 설명                                                        |
| --------------- | ----------------------------------------------------------- |
| `-q, --quiet`   | 진행/상태 메시지(stderr)를 출력하지 않음                    |
| `--debug`       | 디버그 로그 출력                                            |
| `--no-color`    | 색상 출력 비활성화 (`NO_COLOR` 환경변수와 동일)             |
| `--no-input`    | 프롬프트 금지. 입력이 필요하면 안내 후 종료 코드 2로 실패   |
| `--json`        | 결과를 stdout에 기계 판독용 JSON으로 출력                   |
| `-h, --help`    | 도움말 출력                                                |
| `--version`     | 버전 출력                                                   |

### AI 옵션

| 옵션                     | 설명                                                     |
| ------------------------ | -------------------------------------------------------- |
| `--ai-provider <p>`      | LLM 프로바이더 지정 (기본값: `ollama`)                   |
| `--ai-model <m>`         | LLM 모델 지정 (기본값: `llama3`)                         |
| `--ai-base-url <u>`      | LLM 엔드포인트 URL (기본값: `http://localhost:11434`)    |
| `--ai-api-key-file <f>`  | API 키를 파일에서 읽기 (후행 개행 제거)                  |
| `--ai-api-key-stdin`     | API 키를 stdin에서 읽기 (`pass show key \| iggen ...`)   |
| `--ai-api-key <k>`       | (deprecated) 명령줄로 키 전달 — 프로세스 목록에 노출됨   |

### 예시

```bash
# 현재 디렉토리를 스캔하여 자동 생성
iggen

# 언어를 직접 지정
iggen -l python,node

# 기본 OS/에디터 템플릿 없이 rust만 생성
iggen -l rust --no-defaults

# 파일에 쓰지 않고 터미널 출력으로 미리 확인 (dry-run)
iggen --dry-run

# 스크립트에서 결과만 받기 (stdout은 .gitignore 내용, 로그는 stderr)
iggen --dry-run > .gitignore

# 기계 판독 출력
iggen --json --dry-run | jq -r .content

# CI에서 기존 파일 덮어쓰기
iggen -y --output path/to/.gitignore

# LLM(기본 ollama)을 통해 프로젝트 맞춤형 .gitignore 정제 및 생성
iggen --ai

# AI 정제 결과를 파일 변경 없이 미리 확인
iggen --ai --dry-run

# 특정 Ollama 모델 지정하여 실행
iggen --ai --ai-model "llama3.2"

# 원격 Ollama 서버 또는 다른 프로바이더 활용
iggen --ai --ai-base-url "http://192.168.1.100:11434"

# 시크릿을 셸 히스토리에 남기지 않고 전달
printf '%s' "$OPENAI_API_KEY" | iggen --ai --ai-api-key-stdin

# 로컬 템플릿 캐시를 gitignore.io에서 최신화
iggen update
```

## 종료 코드

스크립트와 CI에서 실패 원인을 구분할 수 있도록 코드를 분리합니다.

| 코드 | 의미                                                              |
| ---- | ----------------------------------------------------------------- |
| `0`  | 성공 (파일 생성, dry-run 출력, 사용자가 덮어쓰기를 거부한 경우)   |
| `1`  | 일반 오류 (출력 파일 쓰기 실패, `.gitignore` 내용 생성 실패)      |
| `2`  | 사용법/입력 오류 (잘못된 템플릿 이름, 비대화형에서 확인 필요)     |
| `3`  | 네트워크 오류 (API도 로컬 템플릿 저장소도 사용할 수 없음)         |
| `4`  | 부분 실패 (`update`에서 일부 템플릿만 수집됨)                     |

## 입출력 계약

- **stdout**: 명령의 결과만 출력합니다. `--dry-run`은 `.gitignore` 내용, `--json`은 JSON
  문서 하나를 출력합니다.
- **stderr**: 진행 상황, 경고, 오류, 진행률을 출력합니다. `-q/--quiet`로 억제할 수 있습니다.
- 대화형 프롬프트는 stdin이 터미널일 때만 표시되며, 프롬프트는 stderr로 출력됩니다.
  비TTY/`--no-input` 환경에서는 프롬프트 대신 필요한 플래그를 안내하고 `2`로 실패합니다.
- 파괴적 작업(기존 파일 덮어쓰기)은 대화형 확인을 거치며 `-y/--yes`로 비대화형 실행이
  가능합니다.

## 환경 변수

| 변수                       | 설명                                                        |
| -------------------------- | ----------------------------------------------------------- |
| `IGGEN_API_BASE_URL`       | gitignore.io 엔드포인트 재정의 (사내 미러·목 서버 테스트용) |
| `IGGEN_API_RETRIES`        | 일시적 오류 재시도 횟수 (기본 2, 0~5)                       |
| `IGGEN_AI_PROVIDER`        | LLM 프로바이더 (기본 `ollama`)                              |
| `IGGEN_AI_MODEL`           | LLM 모델명 (기본 `llama3`)                                  |
| `IGGEN_AI_BASE_URL` / `OLLAMA_HOST` | LLM 서버 주소 (기본 `http://localhost:11434`)     |
| `IGGEN_AI_API_KEY` / `OPENAI_API_KEY` | LLM API 키                                |
| `NO_COLOR`, `TERM=dumb`    | 색상 출력 비활성화                                          |
| `XDG_DATA_HOME`            | 사용자 캐시 위치 재정의                                     |

> `IGGEN_API_BASE_URL`에 `http://`를 지정하면 TLS가 꺼집니다. 로컬 테스트 용도로만
> 사용하고, 기본값인 HTTPS에서는 서버 인증서 검증이 항상 켜져 있습니다.

## 오프라인 폴백 (내장 기본값)

gitignore.io가 장애이거나 URL이 변경되거나 서비스가 종료되어도 `iggen`은 계속 동작합니다.

- **내장 기본값**: 감지기가 생성할 수 있는 ~30개 언어 템플릿이 바이너리에 내장되어 있어
  오프라인에서도 항상 사용할 수 있습니다. `tools/fetch_defaults.py`로 재생성합니다.
- **사용자 캐시**: `iggen update`로 gitignore.io의 전체 템플릿 스냅샷을
  `$XDG_DATA_HOME/iggen/templates.json`(macOS: `~/Library/Application Support/iggen`,
  Windows: `%APPDATA%/iggen`)에 저장합니다.

생성 시 **API → 사용자 캐시 → 내장 기본값** 순서로 폴백하며, API가 실패하면 로컬 저장소를
사용하고 로컬에 없는 템플릿은 경고로 알립니다. 로컬에도 없어 아무 내용도 만들 수 없으면
종료 코드 `3`으로 실패합니다.

캐시 파일에는 `schema_version`이 기록되고 **임시 파일 + rename**으로 원자적으로 저장되어,
중단되거나 디스크가 가득 차도 기존 캐시가 깨지지 않습니다.

## LLM 기반 맞춤 정제 (`--ai`)

`--ai` 플래그를 사용하면 gitignore.io 템플릿에만 의존하지 않고, 실제 프로젝트 구조를 심층 분석하여 맞춤형 `.gitignore`를 작성합니다:

- **프로젝트 컨텍스트 분석**: 빌드 도구 파일(`CMakeLists.txt`, `package.json`, `Cargo.toml`, `.env*` 등), 파일 확장자 통계, 샘플 디렉토리 트리를 자동 수집합니다.
- **LLM 라이브러리 연동**: [`LLM_client`](https://github.com/wkqco33/LLM_client) 라이브러리를 통해 로컬 Ollama 또는 다양한 LLM 서버에 연결합니다.
- **안전한 장애 복원 (Graceful Fallback)**: Ollama 서버가 꺼져 있거나 일시적 오류가 발생해도 작업을 중단하지 않고 기본 gitignore.io 템플릿으로 자동 폴백합니다.

## 동작 방식

1. 현재 디렉토리를 재귀 스캔하여 파일 확장자로 언어 감지
2. 기본 템플릿(`visualstudiocode`, `linux`, `macos`, `windows`) 추가 (`--no-defaults`로 비활성화)
3. 템플릿 이름 검증 후 gitignore.io API(또는 로컬 캐시/내장값)에서 템플릿 수신
4. (`--ai` 활성화 시) 프로젝트 파일 구조를 스캔하여 LLM을 통해 맞춤형 `.gitignore`로 정제
5. `.gitignore` 기록 (기존 파일이면 확인 또는 `-y`)

빌드 결과물, 의존성 디렉토리(`.git`, `build`, `node_modules`, `target`, `vcpkg_installed`,
`cmake-build-*`, `.venv` 등)는 감지와 AI 컨텍스트 스캔에서 동일한 규칙으로 제외됩니다.

## Prerequisites

### Linux (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential autoconf autoconf-archive automake libtool pkg-config
```

## 빌드

### 의존성 설치

```bash
vcpkg install
```

### vcpkg 사용 (권장)

```bash
cmake --preset debug
cmake --build --preset debug
```

### 시스템 라이브러리 사용

```bash
cmake --preset debug-native
cmake --build --preset debug-native
```

릴리스 빌드는 `debug` 대신 `release` 프리셋을 사용합니다.

### 경고를 오류로 취급 (CI와 동일)

```bash
cmake --preset debug -B build/strict -DIGGEN_WARNINGS_AS_ERRORS=ON
cmake --build build/strict
```

### 재현 가능한 빌드

- vcpkg 의존성은 `vcpkg.json`의 `builtin-baseline`으로 기준 리비전을 고정합니다.
- `wcppcli`와 `LLM_client`는 `FetchContent`로 가져오며 이동하는 브랜치가 아니라 **커밋 SHA**로
  고정합니다(`CMakeLists.txt`).
- 로컬에서 두 저장소를 수정하며 작업할 때는 소스 디렉토리를 덮어쓸 수 있습니다:

  ```bash
  cmake --preset debug -DFETCHCONTENT_SOURCE_DIR_WCPPCLI=/path/to/wcppcli
  ```

## 지원 범위

| 항목        | 지원                                                             |
| ----------- | ---------------------------------------------------------------- |
| OS          | Linux, macOS, Windows (CI에서 3개 OS 빌드·테스트)                |
| 컴파일러    | GCC, Clang, MSVC (C++17)                                         |
| 빌드 시스템 | CMake 3.20+, Ninja                                               |
| 의존성      | vcpkg: cpp-httplib(openssl), nlohmann-json, cpr, spdlog          |

## 테스트

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

테스트는 **외부 네트워크에 의존하지 않습니다**. HTTP가 필요한 CLI 계약 테스트
(`tests/test_cli.cpp`)는 `127.0.0.1` 루프백 목 서버를 띄워 `IGGEN_API_BASE_URL`로 연결합니다.

## 프로젝트 구조

```text
iggen/
├── include/
│   ├── cli.hpp               # CLI 진입점 (테스트 가능한 run())
│   ├── detector.hpp          # 파일 확장자 → 언어 감지
│   ├── project_scanner.hpp   # --ai용 프로젝트 컨텍스트 스캔
│   ├── ignore_rules.hpp      # 스캐너 공용 제외 규칙
│   ├── api_client.hpp        # gitignore.io API 호출 (재시도/검증)
│   ├── template_store.hpp    # 템플릿 저장소 (load/save/merge/render)
│   ├── default_templates.hpp # 내장 기본값 (자동 생성)
│   ├── ai_refiner.hpp        # LLM 프롬프트/응답 파싱
│   ├── file_writer.hpp       # 파일 출력/덮어쓰기 정책
│   ├── report.hpp            # --json 출력 계약
│   ├── secret_input.hpp      # 시크릿 파일/stdin 입력
│   ├── paths.hpp             # 사용자 데이터 디렉토리 해석
│   ├── exit_code.hpp         # 종료 코드 계약
│   └── version.hpp.in        # 버전 템플릿 (CMake가 생성)
├── src/
│   └── main.cpp              # 진입점 (iggen::run 호출만)
├── tools/
│   └── fetch_defaults.py     # 내장 기본값 재생성 스크립트
├── tests/
│   ├── test_cli.cpp          # CLI 계약(종료 코드·스트림·JSON) E2E
│   ├── test_detector.cpp     # 언어 감지
│   ├── test_ignore_rules.cpp # 제외 규칙
│   ├── test_file_writer.cpp  # 출력/덮어쓰기 정책
│   ├── test_template_store.cpp # 저장소 로드/저장/병합
│   ├── test_paths.cpp        # 경로 해석
│   ├── test_api_client.cpp   # API 입력 검증/엔드포인트 파싱
│   ├── test_project_scanner.cpp # 컨텍스트 스캔
│   ├── test_report.cpp       # JSON 계약 + 시크릿 입력
│   ├── test_ai_refiner.cpp   # LLM 응답 파싱
│   └── smoke_test.cpp        # 헤더 공용 컴파일/링크
├── AGENTS.md             # 에이전트/개발자 개발 가이드
├── CHANGELOG.md          # 변경 이력 (Keep a Changelog)
├── CONTRIBUTING.md       # 기여 가이드
├── CODE_OF_CONDUCT.md    # 행동 강령
├── SECURITY.md           # 보안 정책
└── LICENSE               # MIT 라이선스
```

## 코드 스타일

```bash
clang-format -i src/*.cpp include/*.hpp tests/*.cpp
clang-tidy -p build/debug src/main.cpp
```

`.clang-format` (LLVM, 4-space indent, 100 col) 및 `.clang-tidy` 설정 파일을 따릅니다.
CI는 포맷 검사(`clang-format --dry-run --Werror`), clang-tidy, `-Werror` 빌드를 함께
실행합니다.

## 보안

- gitignore.io API는 HTTPS로 호출하며 서버 인증서를 시스템 CA 저장소로 검증합니다.
- 템플릿 이름은 `[A-Za-z0-9+#._-]{1,64}`로 검증한 뒤에만 URL에 사용합니다.
- LLM API 키는 `--ai-api-key-file`/`--ai-api-key-stdin`/환경변수로 받습니다.
- 릴리스는 CI에서만 수행하고 SHA-256 체크섬, SPDX SBOM, 빌드 provenance를 함께 제공합니다.
- 보안 취약점 신고는 [SECURITY.md](SECURITY.md)를 참고하세요.

## 라이선스

[MIT](LICENSE) 라이선스로 배포됩니다. 기여는 [CONTRIBUTING.md](CONTRIBUTING.md)를 참고하세요.

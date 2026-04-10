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
iggen [options]
```

| 옵션 | 설명 |
|------|------|
| `-l, --lang <langs>` | 쉼표로 구분된 언어 목록 지정 (자동 감지 대신 사용) |
| `--no-defaults` | 기본 템플릿(visualstudiocode, linux, macos, windows) 제외 |
| `-o, --output <file>` | 출력 파일 경로 지정 (기본값: `.gitignore`) |
| `-h, --help` | 도움말 출력 |

### 예시

```bash
# 현재 디렉토리를 스캔하여 자동 생성
iggen

# 언어를 직접 지정
iggen -l python,node

# 기본 OS/에디터 템플릿 없이 rust만 생성
iggen -l rust --no-defaults

# 출력 파일 경로 지정
iggen -o path/to/.gitignore
```

## 동작 방식

1. 현재 디렉토리를 재귀 스캔하여 파일 확장자로 언어 감지
2. 기본 템플릿(`visualstudiocode`, `linux`, `macos`, `windows`) 추가 (`--no-defaults`로 비활성화)
3. gitignore.io API에서 템플릿 조합을 가져와 `.gitignore` 생성

빌드 결과물, 의존성 디렉토리(`.git`, `build`, `node_modules`, `target` 등)는 스캔에서 제외됩니다.

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

## 테스트

```bash
ctest --preset debug
```

## 프로젝트 구조

```
iggen/
├── include/
│   ├── detector.hpp      # 파일 확장자 → 언어 감지
│   ├── api_client.hpp    # gitignore.io API 호출
│   └── file_writer.hpp   # 파일 출력
├── src/
│   └── main.cpp          # CLI 진입점 (wcppcli 기반)
├── tests/
│   ├── smoke_test.cpp
│   └── test_detector.cpp # 언어 감지 단위 테스트
└── wcppcli/              # CLI 프레임워크 서브모듈
```

## 코드 스타일

```bash
clang-format -i src/*.cpp include/*.hpp tests/*.cpp
clang-tidy src/*.cpp -- -std=c++17
```

`.clang-format` (LLVM, 4-space indent, 100 col) 및 `.clang-tidy` 설정 파일을 따릅니다.

# Security Policy

## Reporting a Vulnerability

보안 취약점을 발견하시면 공개 이슈 대신 아래 채널로 신고해 주세요:

- GitHub Security Advisory: <https://github.com/wkqco33/iggen/security/advisories>

신고 내용은 가능한 한 상세히 작성해 주세요 (영향 범위, 재현 방법, 제안된 수정 등).
신고 접수 후 7일 이내에 응답을 드리며, 수정 및 공개 일정을 안내드립니다.

## 보안 고려사항

- gitignore.io API는 HTTPS로 호출하며, 서버 인증서를 시스템 CA 저장소로 검증합니다
  (`include/api_client.hpp`의 `enable_server_certificate_verification(true)`).
  `IGGEN_API_BASE_URL`로 `http://`를 지정하면 TLS가 꺼지므로 로컬 테스트 용도로만
  사용하세요.
- 템플릿 이름은 URL에 사용되기 전에 `[A-Za-z0-9+#._-]{1,64}` 패턴으로 검증되어 경로 주입을
  차단합니다. 검증되지 않은 입력을 API 경로에 전달하지 마세요.
- LLM API 키는 프로세스 목록과 셸 히스토리에 남지 않도록 `--ai-api-key-file` 또는
  `--ai-api-key-stdin`(또는 `IGGEN_AI_API_KEY`/`OPENAI_API_KEY` 환경변수)으로 전달하세요.
  `--ai-api-key` 플래그는 deprecated이며 경고를 출력합니다.
- 사용자 캐시(`templates.json`)는 임시 파일 + rename으로 원자적으로 기록되어 부분 기록된
  파일이 로드되지 않습니다.
- 릴리스는 CI에서만 생성하며 SHA-256 체크섬, SPDX SBOM, 빌드 provenance attestation을 함께
  배포합니다.

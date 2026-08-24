# Security Policy

## Reporting a Vulnerability

보안 취약점을 발견하시면 공개 이슈 대신 아래 채널로 신고해 주세요:

- GitHub Security Advisory: https://github.com/wkqco33/iggen/security/advisories
- 이메일: (유지보수자 이메일)

신고 내용은 가능한 한 상세히 작성해 주세요 (영향 범위, 재현 방법, 제안된 수정 등).
신고 접수 후 7일 이내에 응답을 드리며, 수정 및 공개 일정을 안내드립니다.

## 보안 고려사항

- 이 도구는 gitignore.io API를 HTTPS로 호출합니다. 현재 SSL 인증서 검증이 비활성화되어
  있으므로, 배포/공개 전에 `include/api_client.hpp`에서
  `enable_server_certificate_verification(true)` 및 CA 번들 설정을 활성화하는 것을 권장합니다.
- 이 도구는 사용자 입력을 받아 API 경로에 사용합니다. 경로 주입을 방지하기 위해
  템플릿 이름은 검증 없이 그대로 사용되므로, 신뢰할 수 없는 입력을 전달하지 마세요.

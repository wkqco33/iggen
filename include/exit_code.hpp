#pragma once

namespace iggen {

// CLI 종료 코드 계약. README의 "종료 코드" 표와 항상 동기화한다.
// 새 실패 경로를 추가할 때는 매직 넘버 대신 이 열거형을 사용한다.
enum class ExitCode : int {
    Ok = 0,
    Error = 1, // 일반 실패 (출력 파일 쓰기 실패, .gitignore 내용 생성 실패 등)
    UsageError = 2, // 사용법/입력 오류 (잘못된 템플릿 이름, 비대화형에서 확인 필요 등)
    NetworkError = 3,   // 네트워크 실패 (API도 로컬 저장소도 사용할 수 없음)
    PartialFailure = 4, // 부분 실패 (`update`에서 일부 템플릿만 수집됨)
};

inline auto to_int(ExitCode code) -> int {
    return static_cast<int>(code);
}

} // namespace iggen

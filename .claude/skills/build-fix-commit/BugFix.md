---
name: verify-loop
description: "Self-verification loop for all code generation. Use this skill EVERY time you write, modify, or fix code of any kind — scripts, apps, functions, bug fixes, refactors — even if the user only says 'make', 'fix', or 'add' something. After generating code, you must build/compile it, run tests, and if anything fails, diagnose, fix, and re-run until everything passes. Never declare code 'done' without executing it. Triggers include 코드 작성, 코드 생성, 버그 수정, 기능 추가, 리팩토링, 'write code', 'fix bug', 'implement', 'build', 'test', or any request whose deliverable is working code."
---

# 코드 자가 검증 루프 (Verify Loop)

## 핵심 원칙

**실행해보지 않은 코드는 완성된 코드가 아니다.** 코드를 생성하거나 수정한 뒤에는 반드시 직접 빌드하고 테스트해서 동작을 확인한 후에만 작업을 완료로 선언한다. "아마 될 것이다"는 금지. 눈으로 검토하는 것만으로는 충분하지 않다 — 반드시 실제로 실행한다.

## 워크플로우

아래 루프를 **모든 검증 단계가 통과할 때까지** 반복한다.

```
코드 작성/수정
   ↓
① 빌드/컴파일 → 실패 시 ④로
   ↓
② 테스트 실행 → 실패 시 ④로
   ↓
③ 자체 검토(린트, 엣지 케이스) → 문제 발견 시 ④로
   ↓
✅ 완료 보고
   
④ 원인 분석 → 수정 → ①부터 다시
```

### 1단계: 빌드/컴파일 확인

코드를 작성한 직후 가장 먼저 빌드 또는 문법 검사를 실행한다.

| 언어/환경 | 명령 예시 |
|---|---|
| Python | `python -m py_compile <파일>` 또는 직접 실행 |
| Node.js / JS | `node --check <파일>` 또는 직접 실행 |
| TypeScript | `npx tsc --noEmit` |
| Go | `go build ./...` |
| Rust | `cargo build` 또는 `cargo check` |
| Java | `javac` / `./gradlew build` / `mvn compile` |
| C/C++ | `gcc`/`g++` `-Wall -Wextra` 옵션 포함 |
| 웹 프로젝트 | `npm run build` (package.json의 스크립트 확인) |

프로젝트에 이미 빌드 스크립트가 있다면(`package.json`, `Makefile`, `pyproject.toml` 등) 그것을 우선 사용한다.

### 2단계: 테스트 실행

**기존 테스트가 있는 경우:** 프로젝트의 테스트 러너로 전체 테스트를 실행한다 (`pytest`, `npm test`, `go test ./...`, `cargo test` 등). 수정한 부분만이 아니라 **전체 테스트를 실행**해서 회귀(regression)가 없는지 확인한다.

**테스트가 없는 경우:** 핵심 동작을 검증하는 테스트를 직접 작성해서 실행한다. 최소한 다음을 포함한다:
- 정상 입력에 대한 기대 출력 (happy path)
- 경계값/엣지 케이스 (빈 입력, 0, 음수, 매우 큰 값, 특수문자 등)
- 오류 입력에 대한 예외 처리

테스트 프레임워크 설치가 어려운 환경이라면 간단한 실행 스크립트(assert 기반)로라도 검증한다. 실행 파일이라면 실제 사용 시나리오대로 직접 실행해서 출력을 확인한다.

### 3단계: 자체 검토

빌드와 테스트가 통과했다면 마지막으로 점검한다:
- 린터가 있으면 실행 (`ruff`, `eslint`, `go vet`, `cargo clippy` 등)
- 사용자의 요구사항을 빠짐없이 충족했는지 원래 요청과 대조
- 하드코딩된 임시 값, 디버그 출력(print/console.log), 사용하지 않는 import 제거
- 테스트가 통과했더라도 테스트가 검증하지 못하는 부분(동시성, 리소스 해제 등)이 없는지 확인

### 4단계: 버그 수정 (실패 시)

1. **에러 메시지를 끝까지 읽는다.** 스택 트레이스의 첫 발생 지점을 찾는다.
2. **원인을 파악한 뒤 수정한다.** 증상만 가리는 수정(예: try/except로 감싸서 숨기기)은 금지.
3. 수정 후 **반드시 1단계(빌드)부터 다시** 실행한다. 테스트만 다시 돌리지 말 것 — 수정이 빌드를 깨뜨릴 수 있다.
4. 원인 파악이 어려우면 디버그 출력이나 최소 재현 코드를 추가해 좁혀 들어간 뒤, 확인이 끝나면 디버그 코드를 제거한다.

## 반복 한계와 탈출 조건

- **같은 오류가 3회 연속 반복되면** 같은 방식의 수정을 멈추고 접근 방식 자체를 바꾼다(다른 라이브러리, 다른 알고리즘, 구조 재설계).
- **전체 루프가 10회를 넘으면** 무한 루프로 판단하고 중단한다. 지금까지 시도한 내용, 남은 문제, 가능한 해결 방향을 정리해서 사용자에게 보고하고 판단을 요청한다.
- 환경 제약(네트워크 차단, 패키지 설치 불가, 실행 환경 부재 등)으로 실행 검증이 **불가능한 경우에만** 그 사실을 명시하고, 대신 문법 검사와 정적 검토라도 수행한 뒤 "실행 검증은 하지 못했다"고 솔직하게 보고한다. 검증하지 않고 검증한 것처럼 말하지 않는다.

## 완료 보고 형식

모든 검증이 통과하면 다음을 간단히 보고한다:

```
✅ 검증 완료
- 빌드: 통과 (사용한 명령)
- 테스트: N개 통과 / 0개 실패 (사용한 명령)
- 수정 이력: 발견된 버그와 수정 내용 요약 (없으면 "첫 실행에서 통과")
```

버그를 수정한 경우, 무엇이 왜 문제였고 어떻게 고쳤는지 한두 문장으로 설명한다.

## 예시

**예시 1 — 첫 실행에서 실패한 경우:**

```
[Python 함수 작성]
$ pytest test_parser.py
→ FAILED: IndexError (빈 문자열 입력 시)
[원인 분석: 빈 입력 가드 누락 → 수정]
$ python -m py_compile parser.py   # 빌드부터 재확인
$ pytest test_parser.py
→ 5 passed
✅ 검증 완료 — 빈 문자열 처리 버그 1건 수정
```

**예시 2 — 테스트가 없는 프로젝트:**

```
[Node.js CLI 도구 작성]
$ node --check cli.js              # 문법 확인
[assert 기반 검증 스크립트 test_cli.js 작성]
$ node test_cli.js                 # 정상/엣지/오류 케이스 검증
$ node cli.js --help               # 실제 사용 시나리오 실행
✅ 검증 완료 — 첫 실행에서 통과
```

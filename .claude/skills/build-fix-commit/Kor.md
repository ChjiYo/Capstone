---
name: korean-docs
description: Use this skill whenever creating, writing, or generating any Markdown (.md) file — including CLAUDE.md via /init, README.md, documentation, notes, reports, or any other .md output. Always write the body content of the Markdown file in Korean (한국어), regardless of the language used in the source code, code comments, or the user's own request. Trigger this even if the user asks in English or the codebase is entirely in English — the output .md file content itself must still be in Korean.
---

# 한국어 마크다운 작성 규칙

## 지시사항

1. 새로 생성하거나 수정하는 모든 `.md` 파일의 설명 문구, 제목, 본문 텍스트는 **한국어**로 작성한다.
2. 다음 요소는 번역하지 않고 원문(영어) 그대로 유지한다:
   - 코드 블록 내부의 코드
   - 파일 경로, 명령어(CLI command)
   - 변수명, 함수명, 클래스명, 패키지/라이브러리 이름
   - 고유명사(회사명, 제품명, 프레임워크 이름 등)
3. 소스 코드의 주석이나 커밋 메시지가 영어라도, 이를 요약·설명하는 문서 내용은 한국어로 번역해서 작성한다.
4. `/init` 명령으로 생성되는 `CLAUDE.md`, `README.md` 등 프로젝트 문서 전반에 동일하게 적용한다.
5. 사용자가 영어로 질문하거나 요청하더라도, 결과물이 `.md` 파일이라면 이 규칙을 우선 적용한다.

## 예시

**나쁜 예 (영어로 작성됨):**
```markdown
## Project Overview
This project is a REST API server built with Express.js.
```

**좋은 예 (한국어로 작성, 기술 요소는 유지):**
```markdown
## 프로젝트 개요
이 프로젝트는 Express.js로 구축된 REST API 서버입니다.
```

## 참고

- 이 스킬은 `/init`뿐 아니라 Claude Code가 `.md` 파일을 생성하는 모든 상황(문서화, 요약, 노트 작성 등)에 적용된다.
- 더 확실하게 적용하고 싶다면, 이 내용을 `~/.claude/CLAUDE.md`(전역 메모리 파일)에도 함께 명시하는 것을 권장한다. CLAUDE.md는 스킬 매칭 없이 항상 컨텍스트에 로드되기 때문에 누락 없이 적용된다.

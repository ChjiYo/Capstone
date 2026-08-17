# 12단계: 스토리 UI 제작 명세 (WBP · 사용자 에디터 작업)

## 목표

11단계에서 만든 위젯 C++ 클래스에 짝이 되는 **위젯 블루프린트 3종**을 에디터에서 제작한다. 이름만 정확히 맞추면 `BindWidgetOptional`로 자동 연결되므로, 아래 트리와 값을 그대로 따라 만들면 된다.

| C++ 클래스 | WBP | 유형 | 상태 |
|---|---|---|---|
| `UUI_Dialogue` | `WBP_Dialogue` | 하단 대화창 (모달 A 변형) | 신규 |
| `UUI_ChapterTitle` | `WBP_ChapterTitle` | 전체 화면 오버레이 | 신규 |
| `UUI_Ending` | `WBP_Ending` | 모달 다이얼로그 (A) | 기존 수정 |

## 선행 단계

11단계 코드가 **빌드까지 끝난 뒤에** 진행한다. 클래스가 없으면 Parent Class로 고를 수 없다.

## 읽어야 할 파일

`UI_Dialogue.h`, `UI_ChapterTitle.h`, `UI_Ending.h` — 바인딩 이름은 항상 헤더에서 최종 확인한다.

---

## WBP_Dialogue 제작 명세

**부모 클래스**: `UUI_Dialogue` · **저장 위치**: `Content/UE_Asset/UI/` · **유형**: 하단 대화창(모달 A 변형 — 딤은 나레이션일 때만)

### 1. 위젯 트리

```
[Root] Canvas Panel
 ├─ NarrationBackground (Image)        ← 나레이션 전용 전체 화면 딤. C++가 켜고 끔(기본 숨김)
 └─ DialogueRoot (Size Box)            ← 대사창 크기를 해상도와 무관하게 고정
      └─ ClickArea (Overlay)           ← 버튼을 패널 뒤에 깔아 "창 아무 데나 클릭 = 다음 줄"
           ├─ NextButton (Button)      ← 스타일 전부 투명. Overlay 아래층이라 패널에 가려 보이지 않음
           └─ MainPanel (Border)       ← 실제로 보이는 검은 판. 클릭이 버튼까지 통과하도록 Not Hit-Testable
                └─ Row (Horizontal Box)
                     ├─ PortraitImage (Image)      ← Size Box로 감싸지 않는다: 숨겨질 때 슬롯째 접혀야 하므로
                     └─ TextColumn (Vertical Box)  ← 남은 폭 전부 차지 (Fill 1.0)
                          ├─ SpeakerText (TextBlock)
                          ├─ LineText (TextBlock)
                          └─ HintRow (Horizontal Box)   ← 힌트를 오른쪽 아래에 붙이기 위한 줄
                               └─ ContinueHintText (TextBlock)
```

**굵게 표시된 바인딩 대상**: `NarrationBackground`, `PortraitImage`, `SpeakerText`, `LineText`, `ContinueHintText`, `NextButton` — **이름 변경 금지** (대소문자까지 일치해야 함). `DialogueRoot`/`ClickArea`/`MainPanel`/`Row`/`TextColumn`/`HintRow`는 레이아웃용이라 이름이 자유롭지만 문서와 맞춰두면 나중에 찾기 쉽다.

### 2. 노드별 상세 값

| 위젯 | 앵커 / Alignment | Position / Size | 색·투명도 | 폰트·기타 |
|---|---|---|---|---|
| `NarrationBackground` | 전체 채움 (0,0)~(1,1) | 오프셋 전부 0 | 검정 (0, 0, 0, **0.75**) | Visibility 기본값 **Collapsed** (C++가 나레이션일 때만 켠다) |
| `DialogueRoot` | 하단 중앙 (0.5, 1.0) / Align (0.5, 1.0) | Position (0, -48), Width Override **1100**, Height Override **220** | — | 화면 하단에서 48px 띄운다 |
| `NextButton` | (Overlay 전체 채움) | HAlign/VAlign 모두 Fill | Normal/Hovered/Pressed **전부 A 0** (완전 투명) | 대사창 어디를 눌러도 다음 줄. 보이는 버튼이 아니다 |
| `MainPanel` | (Overlay 전체 채움) | Padding 24 전 방향 | Brush (0.04, 0.04, 0.04, **0.85**) | **Visibility = Not Hit-Testable (Self & All Children)** ← 이걸 빼면 클릭이 버튼에 닿지 않는다 |
| `PortraitImage` | Row 슬롯 Auto / VAlign Top | Brush Image Size **140 × 140**, 슬롯 Padding 오른쪽 16 | 틴트 흰색 (1,1,1,1) | 초상화가 없으면 C++가 Collapsed 처리 → 슬롯이 접히며 텍스트가 폭을 가져간다 |
| `SpeakerText` | TextColumn 슬롯 Auto | Padding 아래 8 | 골드 (0.78, 0.63, 0.29, 1.0) | 크기 **20**, Shadow (1,1) 검정 A 0.8 |
| `LineText` | TextColumn 슬롯 **Fill 1.0** | — | 아이보리 (0.91, 0.89, 0.82, 1.0) | 크기 **18**, **Auto Wrap Text 체크**, Line Height 1.15, Shadow (1,1) |
| `HintRow` | TextColumn 슬롯 Auto / HAlign **Right** | — | — | — |
| `ContinueHintText` | — | — | 보조색 (0.60, 0.58, 0.50, 1.0) | 크기 **12**, 기본 문구 `[E] 계속` |

> **폭 1100 / 폰트 18 기준으로 한 줄에 약 45자**가 들어간다. 10단계 대본의 "45자 이내" 규칙은 이 값에서 나온 것이다. 대사창 폭을 줄이면 대본 규칙도 함께 줄여야 한다.

### 3. 에디터 절차

1. `Content/UE_Asset/UI/`에서 우클릭 → User Interface → Widget Blueprint → 이름 `WBP_Dialogue`.
2. Graph 탭 → Class Settings → **Parent Class = `UI_Dialogue`** (이 단계를 빼면 아무것도 바인딩되지 않는다).
3. 위 트리대로 위젯을 배치하고 표의 값을 입력한다. 바인딩 대상 6개는 이름을 정확히 입력한다.
4. Designer 우측 상단 미리보기 해상도를 1920×1080으로 두고 확인한다.
5. `BP_PyGrey` → `StoryComponent` 선택 → Details의 `Story` 카테고리 → **`Dialogue Widget Class = WBP_Dialogue`**, `Dialogue Table = DT_Dialogue`.

### 4. 확인 체크리스트

- [ ] 컴파일 후 Details 창에 `Is Variable` 체크된 위젯 6개가 헤더 이름과 철자까지 같다.
- [ ] PIE에서 하란에게 말 걸면 대사창이 화면 하단에 뜬다.
- [ ] 대사창 **아무 곳이나 클릭**하면 다음 줄로 넘어간다 (안 넘어가면 `MainPanel`의 Not Hit-Testable 설정을 확인).
- [ ] 프롤로그(나레이션)에서 화면 전체가 어두워지고 이름표·초상화가 사라진다.
- [ ] 초상화 텍스처를 지정하지 않은 대사에서 텍스트가 왼쪽까지 자연스럽게 채워진다.
- [ ] 45자짜리 대사가 2줄 안에 들어가고 잘리지 않는다.

---

## WBP_ChapterTitle 제작 명세

**부모 클래스**: `UUI_ChapterTitle` · **저장 위치**: `Content/UE_Asset/UI/` · **유형**: 전체 화면 오버레이 (2.5초 후 자동 제거)

### 1. 위젯 트리

```
[Root] Canvas Panel
 ├─ CardBackground (Image)        ← 화면 전체를 살짝 눌러 글자를 읽히게 하는 얕은 딤
 └─ CardBox (Vertical Box)        ← 라벨/구분선/제목을 세로로 쌓고 통째로 페이드시키는 단위
      ├─ ChapterLabelText (TextBlock)
      ├─ Divider (Image)
      └─ ChapterTitleText (TextBlock)
```

**바인딩 대상**: `ChapterLabelText`, `ChapterTitleText` (+ 애니메이션 `FadeAnim`). 나머지는 레이아웃용.

### 2. 노드별 상세 값

| 위젯 | 앵커 / Alignment | Position / Size | 색·투명도 | 폰트·기타 |
|---|---|---|---|---|
| `CardBackground` | 전체 채움 | 오프셋 0 | 검정 (0, 0, 0, **0.35**) | 게임 화면이 비쳐야 하므로 딤을 약하게 |
| `CardBox` | 중앙 (0.5, 0.5) / Align (0.5, 0.5) | Position (0, -40), Size To Content 체크 | — | 화면 정중앙보다 살짝 위 |
| `ChapterLabelText` | HAlign Center | Padding 아래 8 | 골드 (0.78, 0.63, 0.29, 1.0) | 크기 **20**, Letter Spacing 200 (자간을 벌려 부제처럼) |
| `Divider` | HAlign Center | Width **320**, Height **2**, Padding 위아래 8 | 골드 A **0.6** | — |
| `ChapterTitleText` | HAlign Center | — | 아이보리 (0.91, 0.89, 0.82, 1.0) | 크기 **40**, Shadow (2,2) 검정 A 0.8 |

### 3. 애니메이션 `FadeAnim` (이름 고정)

Animations 패널에서 새 애니메이션을 만들고 **이름을 정확히 `FadeAnim`**으로 한다 (`BindWidgetAnimOptional` 매칭).

| 시간 | 트랙 | 값 |
|---|---|---|
| 0.0s | `CardBox` → Render Opacity | 0.0 |
| 0.0s | `CardBackground` → Render Opacity | 0.0 |
| 0.4s | 둘 다 | 1.0 |
| 1.9s | 둘 다 | 1.0 (유지) |
| 2.4s | 둘 다 | 0.0 |

총 길이 **2.4초** — `UStoryComponent::ChapterTitleDuration`(2.5초)보다 짧아야 페이드아웃이 끝난 뒤 위젯이 제거된다. 카드 표시 시간을 바꾸려면 두 값을 함께 바꾼다.

### 4. 에디터 절차

1. `WBP_ChapterTitle` 생성 → Parent Class = `UI_ChapterTitle`.
2. 트리 구성 + 값 입력 + `FadeAnim` 제작.
3. `BP_PyGrey` → `StoryComponent` → **`Chapter Title Widget Class = WBP_ChapterTitle`**.

### 5. 확인 체크리스트

- [ ] 퀘스트 [수락] 직후 "제 1장 / 살아남은 사람들"이 페이드 인·아웃된다.
- [ ] 카드가 사라진 뒤 화면에 잔상(투명 위젯)이 남아 클릭을 먹지 않는다.
- [ ] `ChapterLabel`을 비워둔 퀘스트에서는 카드가 아예 뜨지 않는다 (11단계 코드 동작).

---

## WBP_Ending 수정 명세

**부모 클래스**: `UUI_Ending` (기존) · **유형**: 모달 다이얼로그(A)

8단계에서 만든 `WBP_Ending`에 **에필로그 문구 한 줄만 추가**한다. C++ 바인딩이 없는 **정적 텍스트**라 코드 변경이 필요 없다.

### 1. 추가 위치

```
MainPanel (Border)
 └─ ContentBox (Vertical Box)
      ├─ (기존) 타이틀 텍스트 "THE END" 등
      ├─ EpilogueText (TextBlock)   ← 여기 추가 (타이틀 아래, 요약 위)
      ├─ (기존) SummaryText
      └─ (기존) RestartButton / QuitButton
```

### 2. 값

| 위젯 | 값 |
|---|---|
| `EpilogueText` | 내용: `루엔은 다시 지어졌다. 종탑은, 세우지 않기로 했다.` |
| 색·폰트 | 아이보리 (0.91, 0.89, 0.82, 1.0), 크기 **18**, Justification **Center**, Auto Wrap Text 체크 |
| 슬롯 | Padding 위 16 / 아래 24 |

> 이 텍스트는 `BindWidgetOptional` 대상이 **아니다** — 바인딩 목록에 없어도 정상이다. 이름은 문서 일관성을 위한 권장값일 뿐이다.

### 3. 확인 체크리스트

- [ ] 보스 처치 → 에필로그 나레이션 3줄 → 엔딩 화면에 위 문구가 보인다.
- [ ] 기존 `SummaryText`(레벨/골드)와 버튼 2개가 그대로 동작한다.

---

## 12단계 완료 기준 (PIE 통합 확인)

- [ ] 신규 게임 → 프롤로그 나레이션 → 1~4장 대사·챕터 카드 → 보스 조우/페이즈2/유언 → 에필로그 → 엔딩까지 **한 번도 끊기지 않고** 재생된다.
- [ ] 대사창·챕터 카드가 기존 HUD(HP/EXP/퀘스트 트래커/스킬바)를 가리지 않는다 (Z-Order: HUD 0 < 대사창 10 < 챕터 카드 20).
- [ ] 대사 중 인벤토리(I)나 상점을 열어도 UI가 겹쳐 깨지지 않는다.
- [ ] 위젯 3종 모두 1920×1080 / 1280×720 두 해상도에서 레이아웃이 무너지지 않는다.

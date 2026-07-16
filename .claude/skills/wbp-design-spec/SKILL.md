---
name: wbp-design-spec
description: "위젯 블루프린트(WBP) 제작 명세서 작성 스킬. UUserWidget C++ 클래스를 새로 만들거나 수정했을 때, 단계 문서의 '에디터 작업'에 위젯 BP 생성이 포함될 때, 또는 사용자가 위젯/UI/WBP/UMG를 어떻게 만들어야 하는지 물어볼 때 반드시 이 스킬을 사용한다. C++ 헤더의 BindWidgetOptional 멤버를 읽어 위젯 트리 구조(래핑), 앵커/위치/크기, 색상/투명도, 폰트, 버튼 상태까지 에디터에서 그대로 따라 만들 수 있는 상세 명세를 출력한다. Triggers: 위젯, WBP, UMG, UI 만들기, 위젯 블루프린트, HUD, 다이얼로그 창, BindWidgetOptional, 에디터 작업 안내."
---

# WBP 제작 명세서 작성 (Capstone 프로젝트 전용)

## 왜 이 스킬이 필요한가

이 프로젝트에서 `.uasset`(위젯 블루프린트)은 바이너리라서 Claude가 직접 만들 수 없고, **사용자가 언리얼 에디터에서 수동으로 제작**한다. C++ 쪽은 `BindWidgetOptional`로 이름만 맞으면 자동 연결되는 구조이므로, 사용자가 헤더를 열어 이름을 일일이 대조하지 않아도 되도록 **에디터에서 그대로 따라 만들 수 있는 완성된 명세서**를 Claude가 항상 함께 제공해야 한다. 명세 없이 "WBP_XXX를 만들어 연결하세요"라고만 안내하는 것은 이 프로젝트에서 미완성 답변이다.

## 작업 순서

1. **짝이 되는 C++ 헤더를 먼저 읽는다** (`Source/Capstone/UI_*.h`). `BindWidgetOptional` 멤버의 **정확한 이름과 타입**이 명세의 뼈대다. 절대 기억에 의존해 이름을 쓰지 말 것 — 대소문자 하나만 달라도 바인딩이 실패한다.
2. **위젯이 열리는 방식을 확인한다** (`CreateWidget` 호출부, `AddToViewport`, 입력 모드 전환 여부). 이것으로 아래 4가지 유형 중 하나를 판정한다.
3. 유형별 래핑 패턴 + 공통 디자인 시스템을 적용해 **출력 템플릿**대로 명세서를 작성한다.
4. 이미 존재하는 위젯을 수정하는 경우, 바뀐/추가된 요소만 다루되 배치 맥락(어느 패널 아래에 넣는지)은 반드시 포함한다.

## 위젯 유형 판정과 래핑 패턴

### 유형 A — 모달 다이얼로그 (상점, 강화, 퀘스트 수락창 등)
NPC 상호작용 등으로 열리고, 열리는 동안 `SetGameplayInputLocked(true)` + 마우스 커서가 켜지는 위젯.

```
[Root] Canvas Panel
 ├─ DimBackground (Image) ─ 앵커: 전체 채움(0,0 ~ 1,1), 오프셋 전부 0
 │    색: (R 0, G 0, B 0, A 0.5) ← 뒤 게임화면을 어둡게 눌러주는 딤 레이어
 └─ MainPanel (Border) ─ 앵커: 중앙(0.5, 0.5), Alignment (0.5, 0.5), Position (0, 0)
      크기: Size To Content 끄고 명시 지정 (다이얼로그 기준 600×500 안팎)
      Brush 색: (R 0.04, G 0.04, B 0.04, A 0.85) ← 짙은 반투명 검정
      Padding: 24 전 방향
      └─ ContentBox (Vertical Box) ← 제목/내용/버튼을 위에서 아래로 쌓는 몸통
```

- 제목 줄은 ContentBox 맨 위에 `TextBlock`(폰트 24, 골드색), 그 아래 구분선으로 `Image`(높이 2, 골드색 A 0.6)를 넣으면 기존 UI와 톤이 맞는다.
- 닫기 버튼(`CloseButton`)은 MainPanel 우상단에 겹치도록 Canvas 직속으로 두거나, 제목 줄 Horizontal Box의 오른쪽 끝에 배치. 크기 40×40.
- 확인/실행 버튼(강화, 수락 등)은 ContentBox 맨 아래에 배치, 크기 200×50, 가운데 정렬.

### 유형 B — HUD 상시 표시 (HP바, EXP바, 퀘스트 트래커 등)
`BeginPlay`에서 `AddToViewport`되고 게임 내내 떠 있는 위젯. 딤 배경 없음, 입력 안 막음.

```
[Root] Canvas Panel
 └─ RootBox (Size Box 또는 Border) ─ 화면 가장자리 앵커에 부착
```

- 코너 앵커 규칙: 좌상단 = 플레이어 상태(HP), 우상단 = 퀘스트 트래커, 하단 중앙 = EXP바. **새 HUD 위젯은 비어 있는 코너를 쓰고, 기존 위젯과 같은 코너에 넣어야 하면 세로로 이어 붙인다** (기존 요소 높이 + 8px 아래).
- 가장자리 여백: 화면 모서리에서 16px 띄운다 (예: 좌상단이면 Position (16, 16), Alignment (0, 0)).
- 배경은 눈에 거슬리지 않게 A 0.4~0.6의 검정, 또는 배경 없이 텍스트에 그림자(Shadow Offset 1,1 / 검정)만.

### 유형 C — 리스트 슬롯 (상점 슬롯, 인벤토리 슬롯 등)
C++에서 `CreateWidget`으로 여러 개 생성돼 `PanelWidget`(ScrollBox/VerticalBox 등)에 `AddChild`되는 위젯.

```
[Root] Size Box ─ Width/Height Override로 고정 크기 (가로 리스트 항목이면 460×64, 정사각 아이콘 슬롯이면 96×96)
 └─ SlotBorder (Border) ─ Brush 색 (0.08, 0.08, 0.08, A 0.9), Padding 8
      └─ Row (Horizontal Box)
           ├─ ItemIcon (Image) ─ 크기 48×48, Size Box로 감싸 고정
           ├─ 텍스트들 (TextBlock) ─ Fill로 남은 폭 차지, VAlign Center
           └─ ActionButton (Button) ─ 오른쪽 끝, 크기 72×40
```

- 슬롯 루트를 Size Box로 고정하는 이유: 부모 패널이 무엇이든(ScrollBox든 WrapBox든) 슬롯이 제멋대로 늘어나지 않게 하기 위함.
- C++ 멤버 타입이 `UPanelWidget*`인 컨테이너(예: `BuyItemsPanel`)는 에디터에서 ScrollBox/VerticalBox 중 아무거나 써도 바인딩된다. 항목이 5개를 넘을 수 있으면 ScrollBox를 권장.

### 유형 D — 월드 부착 (몬스터 머리 위 HP바)
`UWidgetComponent`로 액터에 붙는 위젯. Canvas 없이 최소 구성.

```
[Root] Size Box (예: 120×16)
 └─ ProgressBar
```

- Space는 Screen, Draw Size는 Size Box와 동일하게. 화면 공간이라 폰트/두께는 HUD보다 한 단계 작게.

## 공통 디자인 시스템 (다크 판타지 톤)

| 용도 | 값 (RGBA, 0~1) | 비고 |
|------|----------------|------|
| 패널 배경 | 0.04, 0.04, 0.04, 0.85 | 모달 메인 패널 |
| 슬롯/내부 박스 배경 | 0.08, 0.08, 0.08, 0.9 | 패널보다 한 톤 밝게 |
| 딤 레이어 | 0, 0, 0, 0.5 | 모달 뒤 전체 화면 |
| 강조/제목/테두리 (골드) | 0.78, 0.63, 0.29, 1.0 | #C8A24B |
| 본문 텍스트 (아이보리) | 0.91, 0.89, 0.82, 1.0 | #E8E2D0 |
| 보조 텍스트 (회갈색) | 0.60, 0.58, 0.50, 1.0 | 수량, 단위 등 |
| 부족/불가 (붉은색) | 0.75, 0.22, 0.17, 1.0 | 재화 부족 표시 |
| 가능/성공 (초록) | 0.30, 0.69, 0.31, 1.0 | 충족 상태 표시 |
| HP바 채움 | 0.75, 0.22, 0.17, 1.0 | ProgressBar Fill Color |
| EXP바 채움 | 0.78, 0.63, 0.29, 1.0 | 골드와 동일 |

- **폰트**: 프로젝트 기본(Roboto) 유지. 크기 — 제목 24 / 본문·버튼 16 / 보조(수량·단위) 12. HUD는 각 단계에서 -2.
- **패딩 스케일**: 4 / 8 / 16 / 24만 사용 (임의 수치 금지 — 통일감의 핵심).
- **버튼 상태** (Button 스타일의 각 Brush Tint):
  - Normal: (0.10, 0.10, 0.10, 1.0) + 골드 외곽선 느낌을 원하면 Outline 1px
  - Hovered: (0.20, 0.17, 0.10, 1.0) ← 골드 기운이 도는 밝은 톤
  - Pressed: (0.05, 0.05, 0.05, 1.0)
  - Disabled: C++에서 `SetIsEnabled(false)` 호출 시 자동으로 반투명해지므로 별도 스타일 불요. 단 버튼 안 텍스트가 안 보이면 Disabled Tint의 A를 0.4로.
- 텍스트에는 기본적으로 Shadow Offset (1, 1), Shadow Color 검정 A 0.8을 준다 — 배경이 밝은 씬에서도 읽힌다.

## C++ 연동 규칙 (명세서에 항상 포함할 것)

1. **위젯 이름 = C++ 멤버 이름, 대소문자까지 정확히** (`BindWidgetOptional` 매칭 규칙). 명세서에는 바인딩 대상 위젯 이름을 굵게 표시하고 "이름 변경 금지"를 명시한다.
2. WBP 생성 직후 **Graph 탭 → Class Settings → Parent Class를 해당 C++ 클래스로 지정** — 이걸 빼먹으면 아무것도 바인딩되지 않는다. 저장 위치는 기존 관례대로 `Content/UE_Asset/UI/`.
3. **블루프린트 그래프 작업은 없다**: 버튼 OnClicked, 텍스트 갱신, ProgressBar Percent 등 로직은 전부 C++가 처리한다. 사용자는 레이아웃만 만들면 된다.
4. 바인딩 안 된 요소는 `BindWidgetOptional`이라 조용히 무시된다 — 완성 후 이름 오타 확인이 마지막 체크 항목.
5. 위젯 클래스를 소비하는 쪽 프로퍼티(예: NPC의 `EnhanceWidgetClass`, 플레이어의 `InventoryWidgetClass`)에 새 WBP를 연결하는 절차까지 명세서에 포함한다.

## 출력 템플릿

명세서는 반드시 아래 구조로 출력한다:

```markdown
## WBP_XXX 제작 명세

**부모 클래스**: UUI_XXX · **저장 위치**: Content/UE_Asset/UI/ · **유형**: 모달 다이얼로그(A)

### 1. 위젯 트리
(들여쓰기 트리 — 각 노드에 `이름 (UMG 타입)` 표기, 바인딩 대상은 **굵게**)

### 2. 노드별 상세 값
(표: 위젯 이름 | 앵커/Alignment | Position/Size | 색·투명도 | 폰트/기타)

### 3. 에디터 절차
(생성 → Parent Class 지정 → 트리 구성 → 소비자 프로퍼티 연결, 번호 목록)

### 4. 확인 체크리스트
(바인딩 이름 대조, PIE에서 열림/닫힘, 데이터 갱신 확인)
```

트리의 각 노드에는 그 노드가 **왜 그렇게 감싸져 있는지** 한 줄 설명을 붙인다 (예: "Size Box로 감싸 슬롯 크기 고정"). 사용자가 이유를 알면 변형이 필요할 때 스스로 올바르게 조정할 수 있다.

## 참고 — 기존 위젯 대응표

새 위젯의 이름/배치를 정할 때 기존 관례와 충돌하지 않는지 이 표로 확인한다. (바인딩 이름은 반드시 헤더에서 재확인)

| C++ 클래스 | WBP | 유형 |
|-----------|-----|------|
| UUI_HpBar | WBP_UI_Py_HpBar / WBP_UI_Mon_HpBar | B / D |
| UUI_ExpBar | WBP_ExpBar | B |
| UUI_QuestTracker | WBP_QuestTracker | B |
| UUI_Inventory / UUI_InventorySlot | WBP_UI_Inventory / WBP_UI_InventorySlot | A / C |
| UUI_Quest | WBP_UI_NPC_Quest | A |
| UUI_Shop / UUI_ShopSlot | WBP_Shop / WBP_ShopSlot | A / C |
| UUI_Enhance | WBP_Enhance | A |

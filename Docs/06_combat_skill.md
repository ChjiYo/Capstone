# 6단계: 전투 액션 강화 (스킬 / 쿨다운 / 타격감)

## 목표

기존 전투(콤보 공격, `SkillSlotA/B/C`, 대시)를 "박진감 있는 액션"으로 끌어올린다.

1. 스킬 3종에 **쿨다운**과 **HUD 스킬 슬롯 UI** (쿨다운 시계 표시)를 붙인다.
2. **타격감 연출**: 히트스톱, 카메라 셰이크, 넉백.
3. **회피(대시) 무적 프레임**.

이 단계는 1~5단계와 독립적이므로 병행 구현이 가능하다. 7단계(적응형 AI)가 이 단계의 회피/피격 훅을 사용하므로 7단계보다는 먼저 해야 한다.

## 선행 단계

없음 (기존 전투 코드 위에 얹음).

## 읽어야 할 파일

`Py_Grey.h/.cpp` (`SkillSlotA/B/C`, `RollOrDash`, `WepaonBeginOverlap`, `TakeDamage`), `Py_GreyAnim.h/.cpp`, `EffectDestroy.h`, `AnimNotify_SpawnSkillEffect.h`, `CharBase.h`, `Monster.h/.cpp` (`Hit`, `TakeDamage`).

## 설계

### 1. 스킬 쿨다운

`APy_Grey`에 슬롯별 데이터 구조를 둔다:

```cpp
USTRUCT(BlueprintType)
struct FSkillSlotData
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) UAnimMontage* SkillMontage = nullptr;
    UPROPERTY(EditAnywhere) float Cooldown = 5.f;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UTexture2D> Icon;
    float LastUsedTime = -FLT_MAX;   // 월드 시간 기준
};
// APy_Grey: UPROPERTY(EditAnywhere, Category="Skill") FSkillSlotData SkillSlots[3];  // A/B/C
```

- `SkillSlotA/B/C` 진입부에 공통 검사 함수 `bool TryUseSkill(int32 SlotIndex)` 추가: 쿨다운 중이면 false, 아니면 `LastUsedTime` 갱신 후 몽타주 재생. 기존 이펙트 스폰 경로(`UAnimNotify_SpawnSkillEffect`)는 그대로.
- 남은 쿨다운 조회: `float GetSkillCooldownRemaining(int32 SlotIndex) const` (`BlueprintPure`) — UI가 Tick에서 폴링.

### 2. 스킬 슬롯 HUD — `UUI_SkillBar`

- 슬롯 3개 (아이콘 + 쿨다운 오버레이 + 키 표기 A/B/C… 실제 바인딩 키 확인 후 표기).
- 쿨다운 표시는 머티리얼 원형 마스크 대신 **단순 ProgressBar 또는 반투명 오버레이 + 남은 초 텍스트**로 (에디터 작업 최소화).
- `APy_Grey::BeginPlay`에서 기존 HUD 위젯들(HP바/트래커/EXP바)과 같은 방식으로 생성·추가.

### 3. 타격감 (Game Feel) — 우선순위 순

적용 지점은 두 곳: **플레이어가 몬스터를 때렸을 때**(`WepaonBeginOverlap`에서 데미지 적용 성공 시)와 **플레이어가 맞았을 때**(`APy_Grey::TakeDamage`).

1. **히트스톱**: 명중 순간 아주 짧게 시간 정지. `UGameplayStatics::SetGlobalTimeDilation(World, 0.05f)` 후 0.06~0.1초(실시간) 타이머로 복구. 연출 수치는 `UPROPERTY` 노출. 몽타주 재생과 타이머가 dilation의 영향을 받는 점 주의 — 복구 타이머는 `SetTimer`의 실시간 옵션 또는 짧은 값으로 보정.
2. **카메라 셰이크**: `UCameraShakeBase` 서브클래스(C++ 또는 BP)로 약/강 2종. 명중 시 약, 피격/스킬 적중 시 강. `APlayerController::ClientStartCameraShake`.
3. **넉백**: 몬스터 피격 시 공격자 반대 방향으로 `LaunchCharacter` 소량. 기존 `AMonster::Hit()` 피격 리액션과 함께 동작.
4. (선택) 무기 트레일 나이아가라, 피격 플래시 머티리얼 — 코드 훅만 두고 에셋은 사용자.

### 4. 회피 무적 프레임

- `APy_Grey`에 `bool bInvincible` 추가. `RollOrDash`로 대시 몽타주가 재생되는 동안(시작~`InvincibleDuration`초, `UPROPERTY` 노출, 기본 0.4초) true.
- `APy_Grey::TakeDamage`에서 `bInvincible`이면 데미지 0 처리 후 조기 반환.
- **7단계 연동 훅**: 회피 시작 시점에 "몬스터 공격 중 회피였는지"를 기록해야 하므로, 대시 실행 지점에서 전역 방송 하나를 추가해 둔다:
  `DECLARE_MULTICAST_DELEGATE(FOnPlayerDodge)` / `static FOnPlayerDodge OnPlayerDodge;` (APy_Grey에, `AMonster::OnMonsterKilled` 패턴).

## 에디터 작업 (사용자)

1. `WBP_SkillBar` 위젯 BP 생성 (`BindWidgetOptional` 이름 일치), `APy_Grey` BP에 클래스 연결. 스킬 아이콘 텍스처 지정.
2. 스킬 슬롯 3개에 몽타주/쿨다운 값 입력 (기존 스킬 몽타주 재사용).
3. 카메라 셰이크 BP 2종 생성·연결(C++ 베이스 제공 시 파라미터만 조정).
4. 타격감 수치(히트스톱 강도/시간, 넉백 세기) 플레이하면서 조정.

## 완료 기준 (PIE 테스트)

- [ ] 스킬 사용 후 쿨다운 동안 재사용이 안 되고, HUD 슬롯에 남은 시간이 표시된다.
- [ ] 몬스터 명중 순간 화면이 살짝 멈칫(히트스톱)하고 카메라가 흔들린다.
- [ ] 피격당한 몬스터가 뒤로 밀린다.
- [ ] 대시 직후 몬스터 공격이 겹쳐도 데미지를 받지 않는다 (무적 프레임).
- [ ] 대시 시 `OnPlayerDodge` 방송이 로그로 확인된다 (7단계 준비).

# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

Capstone is an Unreal Engine 5.7 C++ action-RPG project (module name `Capstone`, `CAPSTONE_API`). Gameplay code lives in `Source/Capstone/`; everything else (levels, blueprints, meshes, animations, widgets) is binary content under `Content/UE_Asset/` and is edited through the Unreal Editor, not by hand.

Engine install: `C:\Program Files\Epic Games\UE_5.7`.

## Build / run commands

There is no cross-platform build script or test harness in this repo — this is a Windows-only UE project built via UnrealBuildTool (UBT) or the Editor/Visual Studio.

- **Generate VS project files**: right-click `Capstone.uproject` → "Generate Visual Studio project files", or run UBT's `GenerateProjectFiles` from the engine install.
- **Compile (editor target, dev config)**:
  ```
  "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" CapstoneEditor Win64 Development -project="D:\Unreal_Project\Capstone\Capstone.uproject"
  ```
- **Run the editor**: open `Capstone.uproject` (launches UE 5.7 per `EngineAssociation` in the `.uproject`), or launch `Binaries/Win64` executable directly if already built.
- There are no automated tests (no `Test` target, no CI config) — verification is manual, in-editor (PIE) playtesting.
- Two build targets exist: `Capstone.Target.cs` (Game) and `CapstoneEditor.Target.cs` (Editor).
- Module dependencies (`Capstone.Build.cs`): Core, CoreUObject, Engine, InputCore, EnhancedInput, ApexDestruction, Niagara, GameplayTags, UMG (public); Slate, SlateCore (private).
- Enabled plugins (`Capstone.uproject`): ModelingToolsEditorMode (editor-only), ApexDestruction, CascadeToNiagaraConverter.

## Working with this repo

- **Source of truth for gameplay logic is C++**; Blueprints (`.uasset`) typically subclass the C++ base classes to wire up meshes/animations/widgets and tune editor-exposed properties. When a feature seems "missing" in C++, check whether it's configured on the Blueprint side instead (e.g. `BP_Goblin`, `BP_PyGrey`, `BP_UI_NPC_Quest`) — those are binary and can't be read directly, only referenced by name/class.
- `.uasset`/`.umap` files are binary; do not attempt to view or diff their contents as text. When a task touches both C++ and Blueprints, changes to the `.uasset` files must be made in-editor by the user — you can only change the C++ side.
- Some existing header comments (Korean, written in EUC-KR/CP949) render as mojibake when read as UTF-8 (e.g. `ItemBase.h`, `NPC_Base.h`). This is pre-existing; don't "fix" it as part of unrelated changes, and write new Korean comments as proper UTF-8.
- `GameInfo.h` is a project-wide grab-bag include (logging macros `LOG(...)`, `PrintViewport`, the `FQuestData`/`FQuestReward` structs, `EDir`) and is included from almost every other header — treat it as the closest thing this project has to a common/precompiled header.

## Architecture

### Character hierarchy

```
ACharacter (engine)
 └─ ACharBase          — shared stats (FINFO: hp/attack), EWEAPON-slot weapon map, TakeDamage
     ├─ AMonster        — AI-driven enemy base; MonsterID for quest tracking, HP bar widget component,
     │   │                spawn-point back-reference, attack/hit delegates
     │   ├─ AMon_Goblin  (+ AMon_GoblinAnim)
     │   ├─ AMon_Larvae  (+ AMon_LarvaeAnim)
     │   └─ AMon_Narbash (+ AMon_NarbashAnim)
     ├─ ANPC_Base        — interactable NPC base (overlap sphere → Interact BlueprintNativeEvent)
     │   └─ ANPC_Quest    — holds an FQuestData, shows UUI_Quest on interact, grants reward via UQuestComponent
     └─ APy_Grey (+ ICombatInterface) — the player character; owns UInventoryComponent, UQuestComponent,
                                         HUD widgets (HP bar, inventory, quest tracker), dash montages, weapon-box overlap
```

Animation instances mirror this hierarchy: `UCharBaseAnim` (shared attack-combo/hit/death notify plumbing via `Inher*Notify()` virtual hooks) → `UMonsterAnim` → `UMon_GoblinAnim`/etc., and `UPy_GreyAnim`, `UNPC_BaseAnim`/`UNPC_QuestAnim` on the other branches.

### Combat & damage

- `ICombatInterface` is a minimal UInterface (currently just `SetExplosiveBuffState`) implemented by `APy_Grey`.
- Melee: `AWeapon` (and monster-specific weapon boxes, e.g. `AMon_Goblin::WeaponBox`) drive `UBoxComponent` overlap events that call `TakeDamage` on hit.
- Ranged/AoE: `AEffectDestroy` is the base for all VFX/SFX-driven skill effects (spawns Niagara + sound, self-destructs after `fLifeTime`); `AEffect_Projectile` adds a `UProjectileMovementComponent` + collision sphere for traveling projectiles, `AEffect_Dot` adds a damage-over-time timer against actors overlapping its sphere. `UAnimNotify_SpawnSkillEffect` is the animation-notify hook that spawns an `AEffectDestroy` subclass at a configurable target (player/caster/socket/weapon-socket) during a montage.

### Monster AI (Behavior Tree)

- `AMonsterAIController` possesses monsters and drives their `UBehaviorTree`/blackboard.
- Custom BT nodes: `UBTService_Detect` (perception tick), `UBTTask_TargetMove`, `UBTTask_Dash`, `UBTTask_Attack` (montage-driven attack with abort handling), `UBTDecorator_AttackDist` (min/max range gate), `UBTDecorator_CheckHit` (hit-reaction gate).
- `ASpawnPoint` owns monster (re)spawning (`SpawnType`, `SpawnTime`, infinite/finite mode) and is referenced back from the spawned `AMonster::pSpawnPoint`.

### Quest system

- `FQuestData`/`FQuestReward` (in `GameInfo.h`) describe a quest: id/title/description, reward item+count, and an optional kill objective (`KillTargetID` matched against `AMonster::MonsterID`, `KillTargetCount`, runtime `CurrentKillCount`).
- `AMonster::OnMonsterKilled` is a static multicast delegate broadcast whenever any monster dies.
- `UQuestComponent` (owned by `APy_Grey`) subscribes to `OnMonsterKilled`, tracks progress on the single active quest, and broadcasts `OnQuestTrackerChanged` (Blueprint-assignable) for UI to refresh.
- `ANPC_Quest` holds the quest definition, shows `UUI_Quest` (accept/decline dialog) on interact, and grants the reward through the player's inventory on accept.
- UI split: `UUI_Quest` is the NPC dialog (accept/decline), `UUI_QuestTracker` is the always-on HUD tracker widget (`WBP_QuestTracker`) — both use `BindWidgetOptional` so the paired Blueprint widget can be missing pieces without breaking compilation.

### Inventory & items

- `AItemBase` is a world pickup (overlap sphere + mesh); `PickupItem` transfers `ItemID` into the picker's `UInventoryComponent`.
- `UInventoryComponent::Items` is a flat `TArray<FName>` of item IDs (shared identifier space with `FQuestReward::RewardItemID`).
- `UUI_Inventory`/`UUI_InventorySlot` render the inventory widget; `APy_Grey::ToggleInventory` shows/hides it.

### Portals / level transition

`APortalPoint` overlaps the player and loads `TargetLevelName` — the only cross-level transition mechanism in the project.

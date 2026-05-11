# Technical Inventory - Arena Co-op v1

Статус: результат `P1-01`
Дата: 2026-05-05
Связанный backlog: [PHASE_1_BACKLOG.md](PHASE_1_BACKLOG.md)
Source of truth: [NOTION_GDD.md](NOTION_GDD.md)

## Короткий вывод

Текущий проект уже даёт хороший локальный FPS-фундамент: gunplay, два оружейных архетипа, здоровье, валюта, shop, armory/inventory, HUD, меню и save/load. Это можно переиспользовать для `Arena Co-op v1`.

Главный разрыв: в коде пока нет arena runtime, сетевой сессии, `GameMode/GameState/PlayerState` под волны, lobby, stats, leaderboards и полноценного persistent profile. Текущая архитектура больше похожа на single-player prototype с Blueprint GameMode и локальными компонентами.

Практическое решение: первым кодовым шагом не подключать Steam и не строить весь duo. Сначала нужно зафиксировать минимальную архитектуру `ArenaGameMode`, `ArenaGameState`, `ArenaPlayerState` и только потом делать маленький solo/local skeleton.

## Inventory Table

| Current system | Existing files/assets | Arena Co-op v1 block | Decision | Notes / risks |
| --- | --- | --- | --- | --- |
| Module and plugins | `Project.Build.cs`, `Project.uproject` | Network Foundation, Steam-only PC | Adapt later | `OnlineSubsystem` and `OnlineSubsystemSteam` are not enabled. Do not enable Steam in the first arena skeleton task. |
| Project maps and default mode | `Config/DefaultEngine.ini`, `/Game/Maps/TestLvl`, `/Game/BP_GameMode/BP_GameMode` | Arena map, Arena runtime | Adapt | Default mode is Blueprint-only from config. Need C++ arena classes before swapping default GameMode. |
| Player controller | `AMainPlayerController` | Player input, HUD/menu control, save commands | Reuse with boundaries | Good entry point for local input and overlays. Do not put arena phase/wave authority here. |
| Combat foundation | `UCombatComponent`, `AWeaponBase`, `WeaponLoadoutTypes`, gameplay tags | Gunplay, 2 weapon archetypes | Reuse | Strong local foundation. Needs later replication/authority audit for duo. |
| Health and hit reactions | `UHealthComponent`, `UHitZoneComponent`, `ADamageTestCube` | Threat HP, player death, kill rewards | Adapt | Health is reusable, but death/reward flow must become arena-authoritative. |
| Currency | `UCurrencyComponent` | Runtime economy, rewards | Adapt | Good local wallet. Duo needs per-player runtime currency and server-side reward decisions. |
| Armory / inventory / loadout | `UPlayerArmoryComponent`, `InventoryItemTypes`, inventory widgets | Owned weapons, loadout, intermission prep | Reuse and adapt | Existing component already captures save data. Need split between runtime state and persistent profile. |
| Weapon shop | `AWeaponShopTerminal`, `UWeaponShopWidgetBase`, shop item widgets | Intermission shop | Reuse and gate by phase | UI and purchase flow exist. Arena must decide when shop is available and validate purchases. |
| Interaction | `UInteractionComponent`, `AInteractableActor`, `AWeaponShopTerminal` | Safe zone terminal, interactables | Reuse | Good for local interaction. Multiplayer needs local-only widget input and server-side gameplay action validation. |
| HUD | `UCrosshairWidgetBase`, `WBP_HUD` | HP, ammo, weapon, currency, wave/threat status, teammate status | Adapt | Current HUD covers HP/ammo/currency/weapon. Needs arena state bindings for wave, threats and duo partner. |
| Startup / in-game menus | `UProjectGameViewportClient`, startup/in-game menu widgets | Main menu, save selection, loading, future Solo/Host Duo | Adapt later | Existing menu is save/start oriented. Arena menu should be designed after session/runtime boundaries are set. |
| Save/load subsystem | `UProjectSaveSubsystem`, `UProjectSaveGame`, `ProjectSaveTypes`, save slot UI | Persistent profile, checkpoint, run meta | Adapt with separation | Current save is world snapshot + player state. `UProjectProfileSaveGame` exists but is nearly empty. Arena profile should be separate from world snapshot saves. |
| World persistence | `USaveableActorComponent`, `UProjectWorldSaveProfile` | World actor persistence | Defer for arena threats | Wave threats should usually be runtime-spawned, not long-term saved world actors. Keep this for authored persistent objects if needed. |
| Content foundation | weapons BPs, shop terminal BP, `BP_MonsterDummy`, `TestLvl` | Prototype arena content | Reuse as test content | Enough to test first slices, but not a real arena/threat roster yet. |
| Arena-specific systems | none found | `ArenaGameMode`, `ArenaGameState`, `ArenaPlayerState`, `WaveDefinition`, `ThreatDefinition`, `ArenaRunState`, `ArenaCheckpointState` | Missing | This is the next architecture/code area. |
| Session / lobby / invite | none found | `SessionSubsystem`, private host + invite | Missing | Keep separate from first arena runtime skeleton. |
| Stats / leaderboards | only save `RunMeta` and inventory stat text | local stats, Steam leaderboards | Missing | Needs profile schema before Steam leaderboard integration. |

## First Safe Code Slices

These are candidates after `P1-02` locks the minimal architecture:

1. Add arena type definitions only:
   - `EArenaPhase`
   - `EArenaRunMode`
   - `FArenaRunState`
   - `FArenaWaveState`
   - no behavior, no Steam, no UI.

2. Add C++ arena class skeleton:
   - `AArenaGameMode`
   - `AArenaGameState`
   - `AArenaPlayerState`
   - basic properties and Blueprint-readable state;
   - no wave spawning yet.

3. Add local solo phase flow stub:
   - `Lobby -> Countdown -> Combat -> Intermission -> Result`;
   - manual/debug phase advancement;
   - no threats, no rewards, no shop gating yet.

4. Add minimal wave state:
   - current wave number;
   - alive threat count;
   - completed/failed state;
   - still no Steam and no invite flow.

5. Connect HUD read-only to arena state:
   - show wave number and remaining threats;
   - keep existing HP/ammo/currency bindings.

## Recommended Next Step

Proceed with `P1-02`: write a short architecture note for the minimal responsibilities of `ArenaGameMode`, `ArenaGameState`, and `ArenaPlayerState`.

Important boundaries for `P1-02`:

- `ArenaGameMode` owns server-authoritative phase transitions and wave decisions.
- `ArenaGameState` exposes replicated/readable match state for HUD and UI.
- `ArenaPlayerState` owns per-player run stats that should survive pawn death during a run.
- `UProjectSaveSubsystem` should not become the arena runtime state owner.
- Steam session work should wait until the local/listen-server arena skeleton exists.

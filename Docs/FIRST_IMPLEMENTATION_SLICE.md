# First Implementation Slice - Arena Runtime Skeleton

Статус: выбран и реализован
Дата: 2026-05-05
Source of truth: [NOTION_GDD.md](NOTION_GDD.md)
Архитектурная основа: [ARENA_RUNTIME_ARCHITECTURE.md](ARENA_RUNTIME_ARCHITECTURE.md)

## Решение

Первым code slice выбираем `Arena Runtime Skeleton`.

Это минимальная C++-основа для будущего arena runtime:

- типы arena run;
- `AArenaGameMode`;
- `AArenaGameState`;
- `AArenaPlayerState`;
- Blueprint-readable состояние;
- class defaults, связывающие GameMode с GameState и PlayerState.

Этот slice не должен менять текущий playable prototype. Существующий `BP_GameMode`, `TestLvl`, save/menu flow, HUD и shop остаются как есть.

## Почему этот slice первый

- У проекта уже есть gunplay, weapon shop, inventory, currency, save и HUD.
- Главный отсутствующий слой сейчас - arena runtime ownership.
- Без `GameMode/GameState/PlayerState` любые волны, rewards, HUD wave counters и future duo будут расползаться по controller/UI/subsystem.
- Skeleton можно проверить компиляцией и Blueprint-доступностью без риска сломать текущую карту.

## Scope

### Входит

- создать папку `Source/Project/Arena`;
- добавить `ArenaTypes.h`;
- добавить `ArenaGameMode.h/.cpp`;
- добавить `ArenaGameState.h/.cpp`;
- добавить `ArenaPlayerState.h/.cpp`;
- ввести `EArenaRunMode`, `EArenaPhase`, `EArenaRunResult`;
- ввести `FArenaWaveState`, `FArenaRunState`, `FArenaPlayerRunStats`;
- сделать типы `BlueprintType`;
- сделать классы `Blueprintable` или Blueprint-friendly;
- добавить Blueprint-readable getters;
- в `AArenaGameMode` выставить `GameStateClass` и `PlayerStateClass`;
- подготовить поля так, чтобы позже их можно было сделать replicated без переписывания API.

### Не входит

- Steam OSS;
- session/invite flow;
- leaderboards;
- wave spawning;
- threat definitions;
- rewards/checkpoint commit;
- death rules;
- HUD integration;
- shop gating by phase;
- profile save layer;
- смена `DefaultEngine.ini`;
- замена текущего `BP_GameMode`;
- изменение текущих Blueprint assets.

## Proposed Files

```text
Source/Project/Arena/ArenaTypes.h
Source/Project/Arena/ArenaGameMode.h
Source/Project/Arena/ArenaGameMode.cpp
Source/Project/Arena/ArenaGameState.h
Source/Project/Arena/ArenaGameState.cpp
Source/Project/Arena/ArenaPlayerState.h
Source/Project/Arena/ArenaPlayerState.cpp
```

## Minimal Public API

### `ArenaTypes.h`

Содержит:

- `EArenaRunMode`
- `EArenaPhase`
- `EArenaRunResult`
- `FArenaWaveState`
- `FArenaRunState`
- `FArenaPlayerRunStats`

Минимальные поля:

- phase;
- run mode;
- run result;
- current wave;
- total waves;
- alive threats;
- spawned threats;
- elapsed seconds;
- kills;
- deaths;
- earned currency;
- committed currency;
- reached wave.

### `AArenaGameMode`

Минимальные обязанности в первом slice:

- задать `GameStateClass = AArenaGameState::StaticClass()`;
- задать `PlayerStateClass = AArenaPlayerState::StaticClass()`;
- хранить настройки по умолчанию для `DefaultRunMode` и `TotalWaves`;
- иметь Blueprint-callable/readable методы-заготовки только если они не запускают реальную логику.

Первый slice не должен реализовывать фазовые переходы. Это отдельная следующая задача.

### `AArenaGameState`

Минимальные обязанности в первом slice:

- хранить `FArenaRunState`;
- отдавать текущую фазу;
- отдавать текущий режим;
- отдавать номер волны;
- отдавать количество живых угроз;
- отдавать результат run.

Replication можно подготовить структурно, но поведение network sync не является целью первого slice.

### `AArenaPlayerState`

Минимальные обязанности в первом slice:

- хранить `FArenaPlayerRunStats`;
- отдавать kills/deaths/currency/reached wave;
- иметь clear/reset helper для будущего старта run.

## Definition of Done

- проект компилируется;
- новые классы доступны Blueprint;
- можно создать Blueprint child от `AArenaGameMode`;
- `AArenaGameMode` указывает на `AArenaGameState` и `AArenaPlayerState`;
- текущий `BP_GameMode` и `TestLvl` не изменены;
- нет изменений Steam/OnlineSubsystem;
- нет изменений HUD/menu/shop/save behavior.

## Implementation Result

Реализовано:

- `Source/Project/Arena/ArenaTypes.h`
- `Source/Project/Arena/ArenaGameMode.h/.cpp`
- `Source/Project/Arena/ArenaGameState.h/.cpp`
- `Source/Project/Arena/ArenaPlayerState.h/.cpp`

Проверка:

- `ProjectEditor Win64 Development` собран через `I:\UE_5.7\Engine\Build\BatchFiles\Build.bat`;
- UnrealHeaderTool прошёл новые `UCLASS/USTRUCT/UENUM`;
- `UnrealEditor-Project.dll` успешно собран.

## Проверка

Минимальная проверка после реализации:

- build проекта;
- убедиться, что UnrealHeaderTool проходит новые `UCLASS/USTRUCT/UENUM`;
- убедиться, что `AArenaGameMode`, `AArenaGameState`, `AArenaPlayerState` видны для Blueprint.

## Следующие задачи после skeleton

1. Добавить manual/debug phase flow.
2. Добавить минимальный wave state update.
3. Подключить read-only HUD bindings к `AArenaGameState`.
4. Отдельно решить profile save layer.
5. После local/listen-server runtime перейти к Steam session foundation.

## Решения, которые пока не принимаем

- точные тайминги countdown/intermission;
- формат wave definitions;
- threat spawning;
- economy commit rules;
- profile schema;
- Steam leaderboard metric implementation;
- final arena map setup.

# Arena Runtime Architecture

Статус: результат `P1-02`
Дата: 2026-05-05
Source of truth: [NOTION_GDD.md](NOTION_GDD.md)
Связанные документы: [TECHNICAL_INVENTORY.md](TECHNICAL_INVENTORY.md), [PHASE_1_BACKLOG.md](PHASE_1_BACKLOG.md)

## Цель решения

Зафиксировать минимальную архитектуру arena runtime до первого кода. Этот документ ограничивает первый implementation slice: мы строим основу для `Arena Co-op v1`, но не подключаем Steam, не делаем invite flow, не пишем полноценный wave spawner и не переписываем UI.

## Runtime Ownership

### `AArenaGameMode`

Серверный владелец правил забега.

Отвечает за:

- выбор режима запуска: `Solo` или будущий `Duo`;
- старт и завершение run;
- переходы фаз `Lobby -> Countdown -> Combat -> Intermission -> Result`;
- старт, завершение и провал волны;
- принятие решений о rewards и checkpoint commit;
- регистрацию смерти игрока и угроз;
- создание authoritative событий, которые потом отражаются в `AArenaGameState`;
- будущую интеграцию с listen-server runtime.

Не отвечает за:

- отображение HUD;
- хранение permanent profile;
- Steam session/invite;
- сохранение мира через `UProjectSaveSubsystem`;
- управление input игрока.

### `AArenaGameState`

Публичное состояние текущего arena run для игроков, HUD и UI.

Хранит:

- текущую фазу матча;
- режим run: `Solo` или `Duo`;
- номер текущей волны;
- общее количество волн;
- количество живых угроз;
- время текущей фазы или run;
- флаги `bRunActive`, `bWaveActive`, `bRunCompleted`, `bRunFailed`;
- короткий результат последнего завершённого шага, если он нужен UI.

Отвечает за:

- read-only представление состояния для клиентов;
- Blueprint-readable API для HUD;
- будущую replication точку для фаз, волн и счётчиков.

Не отвечает за:

- принятие решений о фазах;
- выдачу rewards;
- владение профилем;
- загрузку/сохранение слотов.

### `AArenaPlayerState`

Персональное состояние игрока внутри run.

Хранит:

- kills в текущем run;
- deaths в текущем run;
- runtime currency, если она должна отличаться от permanent currency;
- earned rewards до commit;
- committed rewards в рамках завершённых волн;
- достигнутую волну;
- готовность игрока в lobby/intermission для будущего duo;
- статус alive/spectating для будущего duo.

Отвечает за:

- per-player run stats;
- состояние, которое должно переживать смерть pawn внутри run;
- будущую основу для scoreboard/result screen;
- данные, которые позже могут попасть в persistent profile.

Не отвечает за:

- permanent owned weapons;
- permanent best records;
- Steam leaderboard submit;
- inventory drag/drop UI.

## Minimal Types

Первый code slice должен ввести только те типы, которые нужны для устойчивого skeleton.

```cpp
enum class EArenaRunMode : uint8
{
  Solo,
  Duo
};

enum class EArenaPhase : uint8
{
  None,
  Lobby,
  Countdown,
  Combat,
  Intermission,
  Result
};

enum class EArenaRunResult : uint8
{
  None,
  Completed,
  Failed,
  Aborted
};
```

Минимальные структуры:

- `FArenaWaveState`: wave index, total waves, alive threats, spawned threats, completed/failed flag.
- `FArenaRunState`: run mode, phase, result, elapsed time, active wave summary.
- `FArenaPlayerRunStats`: kills, deaths, earned currency, committed currency, reached wave.

Эти структуры должны быть `BlueprintType`, чтобы их можно было читать из UI/Blueprint без дополнительного glue-кода.

## Phase Flow

Минимальный flow:

1. `Lobby`: run ещё не активен, игрок или игроки готовятся.
2. `Countdown`: короткая подготовка к combat.
3. `Combat`: активна текущая волна, угрозы считаются живыми.
4. `Intermission`: волна завершена, можно открыть shop/armory.
5. Повтор `Countdown -> Combat -> Intermission` до последней волны.
6. `Result`: run завершён или провален.

Первый skeleton может реализовать только ручные/debug переходы фаз. Автоматический wave spawning, rewards и UI gating идут отдельными задачами.

## Runtime vs Persistent Profile

Runtime state и permanent profile не должны смешиваться.

Runtime state:

- текущая фаза;
- текущая волна;
- живые угрозы;
- temporary rewards;
- текущая статистика run;
- состояние alive/spectating.

Persistent profile:

- permanent currency;
- owned weapons;
- selected loadout;
- total runs;
- total clears;
- lifetime kills;
- best solo wave/score;
- best duo wave/score;
- last run summary.

Решение: `AArenaGameMode`, `AArenaGameState` и `AArenaPlayerState` не должны напрямую становиться permanent profile system. Они только производят итоговые данные run, которые позже отдельный profile слой сможет сохранить.

## Save System Boundary

Текущий `UProjectSaveSubsystem` полезен, но это world snapshot save/load, а не владелец arena runtime.

Использование:

- оставить для существующего save/load прототипа;
- переиспользовать идеи и структуры для profile, если это выгодно;
- не привязывать фазу волны к quick save/manual save;
- не сохранять runtime-spawned threats как постоянные world actors по умолчанию.

Открытое решение для `P1-05`:

- расширять `UProjectProfileSaveGame`;
- или добавить отдельный `UProjectProfileSubsystem`;
- или разделить save slots и persistent profile полностью.

## Network Boundary

Первый implementation slice должен быть local/solo или listen-server-ready, но без Steam.

Сразу готовить под replication:

- `EArenaPhase`;
- `EArenaRunMode`;
- текущий wave index;
- alive threat count;
- run result;
- per-player run stats.

Оставить local-only до network phase:

- menu button flow;
- actual Steam session creation;
- invite handling;
- Steam leaderboard submit;
- full duo death/spectator implementation.

Главное правило: `AArenaGameMode` принимает решения, `AArenaGameState` и `AArenaPlayerState` отражают их для клиентов.

## First Skeleton Task

Рекомендуемый первый code slice после этого документа:

- добавить папку `Source/Project/Arena`;
- добавить `ArenaTypes.h`;
- добавить `ArenaGameMode.h/.cpp`;
- добавить `ArenaGameState.h/.cpp`;
- добавить `ArenaPlayerState.h/.cpp`;
- выставить class defaults в `AArenaGameMode` на `AArenaGameState` и `AArenaPlayerState`;
- добавить Blueprint-readable свойства и getters;
- не менять `DefaultEngine.ini`;
- не трогать Steam/OnlineSubsystem;
- не подключать HUD;
- проверить компиляцией.

Definition of done:

- проект компилируется;
- классы доступны Blueprint;
- можно создать Blueprint child от `AArenaGameMode`;
- существующий `BP_GameMode` и текущая карта не ломаются.

## Follow-up Tasks

После skeleton:

1. Добавить ручной/debug phase flow.
2. Добавить минимальный wave state без spawn system.
3. Подключить HUD read-only к `AArenaGameState`.
4. Отдельно решить profile save layer.
5. Только после local/listen-server runtime переходить к Steam session foundation.

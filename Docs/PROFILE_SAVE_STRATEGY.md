# Profile Save Strategy

Статус: результат `P1-05`
Дата: 2026-05-06
Source of truth: [NOTION_GDD.md](NOTION_GDD.md)
Связанные документы: [ARENA_RUNTIME_ARCHITECTURE.md](ARENA_RUNTIME_ARCHITECTURE.md), [NETWORK_FOUNDATION_ORDER.md](NETWORK_FOUNDATION_ORDER.md)

## Решение

`Arena progression` фиксируется как отдельный persistent profile, а не как часть текущих quick/manual save slots.

Текущий `UProjectSaveSubsystem` остаётся владельцем legacy snapshot save/load:

- карта;
- pawn transform;
- HP;
- текущая валюта;
- inventory/loadout;
- world actors.

Arena profile становится отдельным слоем для мета-прогресса между run:

- permanent currency;
- owned weapons;
- selected loadout;
- stats;
- records.

## Ownership

### `UProjectSaveGame` / `UProjectSaveSubsystem`

Назначение:

- legacy/world snapshot saves;
- quick save/manual save;
- загрузка конкретного сохранённого состояния карты и игрока;
- поддержка текущего playable prototype.

Не должны владеть:

- arena run phase;
- wave checkpoint state;
- Steam leaderboard records;
- persistent arena profile как продуктовым источником прогрессии.

### `UProjectProfileSaveGame`

Назначение:

- persistent arena profile data;
- мета-прогресс между run;
- данные для local stats и будущего Steam leaderboard submit.

Текущий класс уже существует, но почти пустой. Его нужно расширять в отдельном future implementation task.

### Future `UProjectProfileSubsystem`

Будущий владелец profile операций:

- load profile;
- save profile;
- create default profile;
- commit arena run result;
- expose profile data to menu/profile UI;
- provide local stats for leaderboard submit.

`AArenaGameMode`, `AArenaGameState` и `AArenaPlayerState` не должны напрямую сохранять профиль на диск. Они производят authoritative runtime/result data, а profile subsystem применяет итог.

## Explicit Non-Goals

На этом этапе не делаем:

- миграцию текущих quick/manual save slots в arena profile;
- сохранение arena phase/wave/threat state в profile;
- использование quick save/manual save как checkpoint для arena waves;
- удаление старого save/load;
- изменение текущего `UProjectSaveSubsystem`;
- Steam leaderboard integration;
- profile UI.

## Future Profile Data Contract

Минимальный будущий `UProjectProfileSaveGame` должен хранить:

- schema version;
- saved timestamp;
- permanent currency;
- owned weapons;
- selected loadout;
- total runs;
- total clears;
- lifetime kills;
- best solo wave;
- best solo score;
- best duo wave;
- best duo score;
- last run summary.

Типы данных должны переиспользовать существующие оружейные и loadout-типы там, где это не связывает profile с world snapshot save:

- weapon references можно хранить через soft class references;
- selected loadout должен описывать слоты и выбранные weapon classes;
- last run summary должен быть компактным record, а не полным `FArenaRunState`.

## Commit Flow

Будущий flow:

1. `AArenaGameMode` завершает wave/run и производит authoritative result data.
2. `AArenaPlayerState` хранит per-run stats.
3. `UProjectProfileSubsystem` получает итог run.
4. `UProjectProfileSubsystem` обновляет `UProjectProfileSaveGame`.
5. `UProjectProfileSubsystem` сохраняет profile slot.
6. Leaderboard layer позже читает local stats/profile records и отправляет нужный leaderboard metric.

`UProjectSaveSubsystem` в этот flow не вовлекается.

## Checkpoint Rule

Arena checkpoint commit означает commit прогрессии в profile, а не quick/manual snapshot save.

Правило:

- незавершённая волна не коммитит rewards в profile;
- успешный checkpoint/run result может обновить permanent currency, unlocks и stats;
- runtime-spawned threats не сохраняются как permanent world actors;
- quick/manual save slots не должны менять arena profile.

## Relationship To Existing Prototype

Старый save/load остаётся полезным для текущего single-player prototype и технической проверки world persistence.

Для Arena Co-op v1 он считается legacy/context, пока отдельный profile layer не реализован.

Это снижает риск: можно продолжать пользоваться старым прототипом, не превращая его save slots в долгосрочный продуктовый профиль.

## Acceptance Criteria

Для `P1-05`:

- этот документ существует;
- [PHASE_1_BACKLOG.md](PHASE_1_BACKLOG.md) помечает `P1-05` как `done`;
- текущие save slots явно зафиксированы как legacy snapshot saves;
- выбран вариант separate persistent profile;
- следующий шаг переведён на маленький implementation task после Phase 1 decisions.

Для будущей реализации:

- профиль переживает перезапуск игры;
- quick/manual save slots не влияют на arena profile;
- profile commit происходит только после успешного arena checkpoint/run result;
- local stats могут быть использованы Steam leaderboards позже.

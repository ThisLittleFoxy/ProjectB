# Network Foundation Order

Статус: результат `P1-04`
Дата: 2026-05-05
Source of truth: [NOTION_GDD.md](NOTION_GDD.md)
Опирается на: [ARENA_RUNTIME_ARCHITECTURE.md](ARENA_RUNTIME_ARCHITECTURE.md), [FIRST_IMPLEMENTATION_SLICE.md](FIRST_IMPLEMENTATION_SLICE.md)

## Решение

Порядок такой:

1. Сначала доводим `Arena Runtime` до local/listen-server-ready состояния.
2. Потом проверяем server-authoritative правила в PIE/listen-server сценарии.
3. Потом подключаем session layer.
4. Только после этого подключаем Steam OSS invite flow.
5. Leaderboards идут после profile/stats решения, не вместе с lobby.

Главное ограничение: Steam setup, invite flow, arena rules, rewards и leaderboards не смешиваются в один task.

## Почему так

Если подключить Steam раньше arena runtime, мы получим сетевую оболочку без устойчивых правил матча. Для `Arena Co-op v1` важнее сначала зафиксировать, что сервер владеет фазами, волнами, смертями и наградами, а клиенты только отображают состояние и отправляют запросы.

Steam должен подключаться к уже понятному runtime, а не становиться местом, где случайно рождаются правила игры.

## Order

### NF-01. Local arena runtime

Статус: done

Цель:

- добавить ручной/debug phase flow;
- обновлять `AArenaGameState` из `AArenaGameMode`;
- обновлять `AArenaPlayerState` из authoritative runtime;
- не подключать Steam;
- не менять текущий `BP_GameMode` по умолчанию.

Проверка:

- проект компилируется;
- можно запустить arena GameMode вручную или через Blueprint child;
- фазы читаются из `AArenaGameState`;
- текущий playable prototype не ломается.

Результат:

- [Local Arena Runtime Flow](LOCAL_ARENA_RUNTIME_FLOW.md)

### NF-02. Listen-server readiness

Статус: done

Цель:

- проверить, что базовые arena phase/run state корректно отражаются клиентам;
- держать phase/wave decisions в `AArenaGameMode`;
- держать player run stats в `AArenaPlayerState`;
- отдельно отметить, какие текущие системы требуют replication audit.

Проверка:

- PIE listen-server сценарий с 1-2 clients;
- клиент видит replicated phase/wave state;
- клиент видит свои replicated run stats.

Результат:

- [Listen-server Readiness](LISTEN_SERVER_READINESS.md)

### NF-03. Gameplay authority audit

Статус: done

Цель:

- определить, какие действия должны стать server-authoritative перед duo:
  - старт run;
  - ready state;
  - старт/конец волны;
  - смерть угрозы;
  - смерть игрока;
  - начисление rewards;
  - checkpoint commit;
  - покупка оружия во время intermission;
  - изменение loadout во время intermission.

Результат:

- короткий список code tasks;
- запрет на перенос gameplay decisions в UI/controller.
- [Gameplay Authority Audit](GAMEPLAY_AUTHORITY_AUDIT.md)

### NF-04. Session subsystem skeleton

Цель:

- добавить project-level session abstraction без Steam-specific логики;
- подготовить команды `HostPrivateSession`, `JoinSession`, `LeaveSession`;
- оставить public matchmaking out of scope;
- не добавлять leaderboards.

Решение по OnlineSubsystem:

- для разработки можно использовать local/listen-server и, если потребуется, `OnlineSubsystemNull`;
- продуктовая цель остаётся Steam-only PC;
- Steam-specific код не должен попадать в arena rules.

### NF-05. Steam OSS private invite

Цель:

- включить нужные Steam/OnlineSubsystem зависимости;
- настроить Steam session host/join;
- реализовать private invite flow;
- не добавлять public matchmaking, browser list или join-in-progress.

Проверка:

- host создаёт private session;
- второй игрок подключается по invite;
- после старта run состав фиксируется.

### NF-06. Leaderboards after profile/stats

Цель:

- подключить Steam leaderboards только после решения `P1-05` по profile/stats;
- не смешивать leaderboard submit с session host/join.

Проверка:

- local stats already produce correct record data;
- Steam submit пишет в `Solo_BestWaveTime` или `Duo_BestWaveTime` по режиму.

## Server-Authoritative From First Gameplay Step

С первого gameplay step сервер должен владеть:

- текущей фазой;
- стартом и завершением run;
- стартом и завершением волны;
- количеством живых угроз;
- смертью угроз;
- смертью игроков;
- выдачей runtime rewards;
- checkpoint commit;
- итогом run.

Клиент может владеть:

- input;
- camera;
- локальным UI;
- локальным drag/drop interaction до отправки gameplay request;
- visual/audio feedback.

## Explicitly Out of Scope For Next Code Task

В следующий code task не входят:

- Steam OSS;
- invite flow;
- leaderboards;
- public matchmaking;
- session browser;
- join-in-progress;
- final profile schema;
- полноценная reward economy;
- permanent save/profile commit.

## Следующий инженерный шаг

Перед сетевой работой нужно сделать небольшой gameplay-runtime шаг:

- manual/debug phase flow в `AArenaGameMode`;
- read-only replicated state в `AArenaGameState`;
- базовый reset/update stats в `AArenaPlayerState`.

После этого можно выбрать первый маленький authority implementation task из [Gameplay Authority Audit](GAMEPLAY_AUTHORITY_AUDIT.md). Рекомендуемый первый task: `GA-01 Ready/Start Run Request`.

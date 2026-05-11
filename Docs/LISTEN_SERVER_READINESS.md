# Listen-server Readiness

Статус: done
Дата: 2026-05-06
Опирается на: [NETWORK_FOUNDATION_ORDER.md](NETWORK_FOUNDATION_ORDER.md), [LOCAL_ARENA_RUNTIME_FLOW.md](LOCAL_ARENA_RUNTIME_FLOW.md)

## Цель

Проверить `NF-02`: базовый `Arena Runtime` должен корректно отражать state с listen-server на client без Steam/session layer.

## C++ Debug Support

Чтобы не собирать временные Blueprint debug actors, в C++ добавлен встроенный debug output:

- `AArenaGameMode` содержит короткий debug phase flow, но он выключен по умолчанию;
- `AArenaGameMode` логирует server-side publish state;
- `AArenaGameState` логирует `SetArenaRunState` на сервере и `OnRep_ArenaRunState` на клиенте;
- `AArenaPlayerState` логирует `SetArenaRunStats`, `ResetArenaRunStats` и `OnRep_ArenaRunStats`;
- enum значения печатаются строками: `Lobby`, `Countdown`, `Combat`, `Intermission`, `Result`.

## Как запускать проверку

1. В тестовой карте выставить `World Settings -> GameMode Override = BP_ArenaGameMode`.
2. Убрать временный `BP_ArenaDebugStateActor`, если он остался на карте.
3. Убрать Blueprint debug timers из `BP_ArenaGameMode`, если они были добавлены вручную.
4. Для повторной проверки включить в `BP_ArenaGameMode`:
   - `bAutoRunDebugPhaseFlow`;
   - `bSeedDebugPlayerStatsOnLogin`, если нужно проверить ненулевые replicated player stats.
5. В PIE выставить 2 players и listen-server.
6. Запустить PIE и смотреть Output Log.

## Ожидаемые GameState строки

На сервере:

```text
LogArenaGameState: Display: SetArenaRunState NetMode=ListenServer Phase=Lobby ...
LogArenaGameMode: Display: PublishArenaRunState NetMode=ListenServer Phase=Lobby ...
LogArenaGameState: Display: SetArenaRunState NetMode=ListenServer Phase=Countdown ...
LogArenaGameState: Display: SetArenaRunState NetMode=ListenServer Phase=Combat ... Wave=1/6 Alive=5 Spawned=5
LogArenaGameState: Display: SetArenaRunState NetMode=ListenServer Phase=Intermission ... Wave=1/6 Alive=0
LogArenaGameState: Display: SetArenaRunState NetMode=ListenServer Phase=Result ... Result=Completed
```

На клиенте:

```text
LogArenaGameState: Display: OnRep_ArenaRunState NetMode=Client Phase=Lobby ...
LogArenaGameState: Display: OnRep_ArenaRunState NetMode=Client Phase=Countdown ...
LogArenaGameState: Display: OnRep_ArenaRunState NetMode=Client Phase=Combat ... Wave=1/6 Alive=5 Spawned=5
LogArenaGameState: Display: OnRep_ArenaRunState NetMode=Client Phase=Intermission ... Wave=1/6 Alive=0
LogArenaGameState: Display: OnRep_ArenaRunState NetMode=Client Phase=Result ... Result=Completed
```

## Ожидаемые PlayerState строки

На сервере:

```text
LogArenaPlayerState: Display: ResetArenaRunStats NetMode=ListenServer ...
LogArenaPlayerState: Display: SetArenaRunStats NetMode=ListenServer ... Kills=... Earned=...
```

На клиенте:

```text
LogArenaPlayerState: Display: OnRep_ArenaRunStats NetMode=Client ... Kills=... Earned=...
```

## Pass Criteria

- client подключается к listen-server;
- `BP_ArenaGameMode` является активным GameMode;
- на client появляются `OnRep_ArenaRunState` строки для `Lobby`, `Countdown`, `Combat`, `Intermission`, `Result`;
- на client появляется `OnRep_ArenaRunStats`;
- Steam/OnlineSubsystemSteam не используется;
- текущий основной `BP_GameMode` не меняется глобально.

## Проверенный результат

Проверка выполнена 2026-05-06 в PIE listen-server сценарии с `BP_ArenaGameMode` и `BP_ArenaTestPlayerController`.

Подтверждено:

- server поднялся как `NetMode=ListenServer`;
- client подключился без последующего `HostClosedConnection` / `ConnectionLost`;
- client получил `OnRep_ArenaRunState` для `Lobby`, `Countdown`, `Combat`, `Intermission`, `Result`;
- client получил `OnRep_ArenaRunStats` с debug stats;
- текущий основной `BP_GameMode` глобально не менялся.

## Cleanup Decision

Текущий C++ debug flow остаётся как opt-in инструмент разработки:

- оставить debug logging как configurable tool;
- оставить явные debug methods в `AArenaGameMode`;
- выключить auto-flow и fake player stats по умолчанию;
- не возвращать отдельный `AArenaDebugStateActor` без новой необходимости.

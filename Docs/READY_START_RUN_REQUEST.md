# Ready / Start Run Request

Статус: done
Дата: 2026-05-06
Опирается на: [GAMEPLAY_AUTHORITY_AUDIT.md](GAMEPLAY_AUTHORITY_AUDIT.md)

## Цель

Добавить первый маленький client-to-server gameplay request path для arena runtime.

Игрок выставляет `Ready`, сервер принимает решение и сам переводит run из `Lobby` в `Countdown`, когда все нужные игроки готовы.

## Что добавлено

### `AArenaPlayerState`

- replicated `bArenaReady`;
- `SetArenaReady(bool)`;
- `IsArenaReady()`;
- `OnRep_ArenaReady`;
- Blueprint event `On Arena Ready Changed`.

### `AArenaGameMode`

- `SetPlayerReady(APlayerController*, bool)`;
- `AreAllPlayersReady()`;
- `StartArenaRun()`;
- `MinimumReadyPlayersToStart`;
- reset ready state при `InitializeArenaRun`;
- reset ready state для нового игрока в `PostLogin`.

### `AMainPlayerController`

- `RequestArenaReady(bool bReady)`;
- `ServerSetArenaReady(bool bReady)` RPC.

## Что не входит

- lobby UI;
- host-only start button;
- Steam/session roster;
- wave spawning;
- combat replication;
- shop/currency/loadout authority;
- переход `Countdown -> Combat`.

## Как проверить в PIE

1. На тестовой карте оставить `GameMode Override = BP_ArenaGameMode`.
2. В `BP_ArenaGameMode` выставить `MinimumReadyPlayersToStart = 2` для 2-player listen-server проверки.
3. В `BP_ArenaTestPlayerController` после `BeginPlay` временно вызвать `RequestArenaReady(true)`.
   - Лучше через небольшой delay, чтобы оба PIE игрока успели подключиться.
4. Запустить PIE:
   - `Number of Players = 2`;
   - `Net Mode = Play As Listen Server`.

## Ожидаемые логи

На сервере:

```text
LogArenaPlayerState: Display: SetArenaReady NetMode=ListenServer ... Ready=true
LogArenaGameMode: Display: SetPlayerReady NetMode=ListenServer ... Ready=true
LogArenaGameMode: Display: All required players ready. Starting arena run. NetMode=ListenServer
LogArenaGameState: Display: SetArenaRunState NetMode=ListenServer Phase=Countdown ...
```

На клиенте:

```text
LogArenaPlayerState: Display: OnRep_ArenaReady NetMode=Client ... Ready=true
LogArenaGameState: Display: OnRep_ArenaRunState NetMode=Client Phase=Countdown ...
```

## Pass Criteria

- ready request от клиента доходит до сервера;
- ready state реплицируется клиенту;
- run стартует только на сервере;
- `Countdown` реплицируется клиенту через `AArenaGameState`;
- старый `BP_GameMode` глобально не меняется;
- Steam/session layer не используется.

## Проверенный результат

Проверка выполнена 2026-05-06 в 2-player PIE listen-server сценарии.

Подтверждено:

- оба игрока выставили ready на сервере;
- сервер дождался `MinimumReadyPlayersToStart = 2`;
- сервер сам запустил run;
- `AArenaGameState` перешёл в `Countdown`;
- клиент получил replicated `Countdown`.

Фактические ключевые строки:

```text
LogArenaPlayerState: Display: SetArenaReady NetMode=ListenServer Player=ThisLittleFoxy-B0AEA Ready=true
LogArenaPlayerState: Display: SetArenaReady NetMode=ListenServer Player=ThisLittleFoxy-8DE39 Ready=true
LogArenaGameMode: Display: All required players ready. Starting arena run. NetMode=ListenServer
LogArenaGameState: Display: SetArenaRunState NetMode=ListenServer Phase=Countdown Mode=Solo Result=None Wave=0/6 Alive=0 Spawned=0
LogArenaGameMode: Display: PublishArenaRunState NetMode=ListenServer Phase=Countdown Mode=Solo Result=None Wave=0/6 Alive=0 Spawned=0 RunActive=true WaveActive=false
LogArenaGameState: Display: OnRep_ArenaRunState NetMode=Client Phase=Countdown Mode=Solo Result=None Wave=0/6 Alive=0 Spawned=0
```

Примечание:

- получение клиентом `Lobby`, а затем `Countdown` является нормальным порядком: client сначала догоняет начальное replicated состояние, затем получает server update после ready/start.

## Следующий шаг

Переходить к `GA-02 Armory Access Model`.

# Local Arena Runtime Flow

Статус: implementation result
Дата: 2026-05-06
Опирается на: [NETWORK_FOUNDATION_ORDER.md](NETWORK_FOUNDATION_ORDER.md), [ARENA_RUNTIME_ARCHITECTURE.md](ARENA_RUNTIME_ARCHITECTURE.md)

## Что сделано

Добавлен минимальный local/debug phase flow в `AArenaGameMode`.

Реализация остаётся в границах `NF-01`:

- без Steam OSS;
- без session/invite flow;
- без HUD integration;
- без wave spawning;
- без rewards/checkpoint commit;
- без save/profile изменений;
- без изменения `DefaultEngine.ini`;
- без замены текущего `BP_GameMode`.

## Public Debug API

`AArenaGameMode` теперь предоставляет Blueprint-callable методы:

- `InitializeArenaRun()`;
- `SetArenaPhaseForDebug(EArenaPhase NewPhase)`;
- `AdvanceArenaPhaseForDebug()`;
- `StartCombatWaveForDebug(int32 WaveNumber, int32 SpawnedThreats)`;
- `CompleteWaveForDebug(bool bSucceeded)`;
- `FinishArenaRunForDebug(EArenaRunResult Result)`.

Эти методы нужны для ручной проверки flow и будущего Blueprint child от `AArenaGameMode`.

## Runtime Behavior

- `BeginPlay()` инициализирует run в `Lobby`.
- `PostLogin()` сбрасывает `AArenaPlayerState` stats для вошедшего игрока.
- `InitializeArenaRun()` публикует начальное `FArenaRunState` в `AArenaGameState`.
- `AdvanceArenaPhaseForDebug()` проходит базовый цикл:
  - `None/Result -> Lobby`;
  - `Lobby -> Countdown`;
  - `Countdown -> Combat`;
  - `Combat -> Intermission`;
  - `Intermission -> Countdown` или `Result`, если волны закончились.
- `StartCombatWaveForDebug()` выставляет текущую волну и debug-счётчики угроз.
- `CompleteWaveForDebug()` переводит успешную волну в `Intermission`, а проваленную в `Result/Failed`.
- `FinishArenaRunForDebug()` завершает run с заданным результатом.

## Проверка

Сборка:

```text
I:\UE_5.7\Engine\Build\BatchFiles\Build.bat ProjectEditor Win64 Development -Project="I:\NewProj\ProjectB\Project.uproject" -WaitMutex -NoUBTMakefiles
```

Результат:

- UnrealHeaderTool прошёл;
- `ArenaGameMode.cpp`, `ArenaGameState.cpp`, `ArenaPlayerState.cpp` скомпилировались;
- `UnrealEditor-Project.dll` собран;
- `Result: Succeeded`.

## Следующий шаг

Следующий маленький шаг: `NF-02 Listen-server readiness`.

Нужно проверить, что replicated `AArenaGameState` и `AArenaPlayerState` корректно видны клиентам в PIE/listen-server сценарии, и зафиксировать найденные gaps перед Steam/session работой.

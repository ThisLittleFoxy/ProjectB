# Threat Death And Rewards

Status: `GA-05` implemented
Date: 2026-05-08
Depends on: [Arena Runtime Currency](ARENA_RUNTIME_CURRENCY.md), [Server-Authoritative Purchase](SERVER_AUTHORITATIVE_PURCHASE.md)

## Decision

Arena threat death rewards are owned by `AArenaGameMode`.

`UHealthComponent` still detects death, but in arena worlds it reports the death
to `AArenaGameMode` instead of granting currency through `UCurrencyComponent`.
The legacy `UCurrencyComponent` reward path remains available outside arena
worlds.

## Runtime Flow

1. `UHealthComponent` detects owner death after damage.
2. If the world has `AArenaGameState`, it treats the death as arena runtime
   flow.
3. On authority, `AArenaGameMode::ReportArenaThreatKilled(...)`:
   - increments killer `AArenaPlayerState` kills;
   - adds `CurrencyRewardOnDeath` to arena spendable/earned currency;
   - decrements `AliveThreats` only while phase is `Combat`;
   - moves `Combat -> Intermission` when the last alive threat reaches zero.

## Rules

- Arena reward requires a valid killer controller and positive reward value.
- Reward can be granted outside `Combat` for debug/test kills.
- Wave counters only change in `Combat`.
- In arena worlds, client-side/local fallback to `UCurrencyComponent` is blocked.

## Verification

Manual PIE reward check:

- start with `Spendable=50000`;
- kill `BP_MonsterDummy`;
- expect `Arena threat killed... Reward=50`;
- expect `SetArenaRunStats ... Kills=... Spendable=50050 Earned=50050`.

Manual wave check:

- start a debug combat wave with `AliveThreats > 0`;
- kill threats;
- expect `AliveThreats` to decrement;
- when it reaches `0`, expect phase `Intermission`.

Legacy fallback check:

- outside `AArenaGameState`, the old log remains valid:
  `HealthComponent: '...' granted +... currency to '...'`.

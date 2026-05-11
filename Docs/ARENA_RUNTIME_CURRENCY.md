# Arena Runtime Currency

Status: `GA-03` implemented
Date: 2026-05-08
Depends on: [Gameplay Authority Audit](GAMEPLAY_AUTHORITY_AUDIT.md), [Armory Access Model](ARMORY_ACCESS_MODEL.md)

## Decision

Arena run currency is owned by `AArenaPlayerState`.

`UCurrencyComponent` remains a legacy/local prototype wallet for the old single-player flow. It is not the source of truth for arena rewards, purchases, or checkpoint commits.

Arena UI reads money directly from `AArenaPlayerState::SpendableCurrency`.
Arena currency is not copied into `UPlayerArmoryComponent`.

## Runtime Fields

`FArenaPlayerRunStats` now replicates:

- `SpendableCurrency` - current arena wallet available for runtime spend requests;
- `EarnedCurrency` - total currency earned during the current run;
- `CommittedCurrency` - checkpointed/secured earned currency;
- existing kill/death/reached-wave stats.

## Server API

`AArenaPlayerState` exposes:

- `AddArenaEarnedCurrency(int32 Amount)` - server-only mutation, increases `SpendableCurrency` and `EarnedCurrency`;
- `TrySpendArenaCurrency(int32 Amount)` - server-only mutation, decreases `SpendableCurrency` when enough currency exists;
- `CommitArenaEarnedCurrency()` - server-only mutation, raises `CommittedCurrency` to at least `EarnedCurrency`;
- `SetArenaRuntimeCurrency(...)` - server-only utility for controlled tests/debug setup.

Non-authority calls are ignored and logged.

## What This Does Not Do

- no weapon purchase request yet;
- no inventory/loadout replication yet;
- no threat-death reward bridge yet;
- no profile save commit yet.

Those remain separate slices:

- `GA-04 Server-Authoritative Purchase`;
- `GA-05 Threat Death And Rewards`;
- future profile commit layer.

## Verification

For a manual PIE debug check, enable
`bRunDebugCurrencyTestOnFirstLogin` in `BP_ArenaGameMode`.

For manual purchase checks, enable `bSeedDebugPlayerStatsOnLogin` and set
`DebugSeedSpendableCurrency` to the desired test value. The seed is applied
after `InitializeArenaRun()` so it is not wiped by clean run startup.

The test runs once on the first server-side `PostLogin`, snapshots the player's
current stats, checks the runtime currency API, then restores the original
stats.

Expected log:

- `LogArenaCurrencyDebug: Display: Currency debug test started ...`;
- `Reset OK Spendable=0/0 Earned=0/0 Committed=0/0`;
- `Add 100 OK Spendable=100/100 Earned=100/100 Committed=0/0`;
- `Spend 35 OK Spendable=65/65 Earned=100/100 Committed=0/0`;
- `Spend 999 rejected OK`;
- `Commit OK Spendable=65/65 Earned=100/100 Committed=100/100`;
- `Currency debug test PASSED`.

Expected listen-server signal after rewards/purchases are wired later:

- server logs `SetArenaRunStats ... Spendable=... Earned=... Committed=...`;
- client logs `OnRep_ArenaRunStats ... Spendable=... Earned=... Committed=...`;
- no arena decision depends on `UCurrencyComponent`.

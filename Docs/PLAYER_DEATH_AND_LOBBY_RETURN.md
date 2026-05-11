# Player Death And Lobby Return

Status: `GA-06` implemented
Date: 2026-05-08
Depends on: [Threat Death And Rewards](THREAT_DEATH_AND_REWARDS.md)

## Decision

For the current prototype, arena player death returns the run to `Lobby`.

The dead player loses arena runtime gains:

- `SpendableCurrency = 0`;
- `EarnedCurrency = 0`;
- `CommittedCurrency = 0`.

Future looted arena items should follow the same rule until a profile/checkpoint
commit layer is implemented.

## Runtime Flow

1. `UHealthComponent` detects an arena player pawn death.
2. It suppresses the generic hide/destroy death behavior for that pawn.
3. `AArenaGameMode::ReportArenaPlayerDied(...)`:
   - increments `Deaths`;
   - marks the player dead briefly;
   - clears arena runtime rewards;
   - returns `FArenaRunState` to `Lobby`;
   - resets player ready/alive state for the new lobby state.
4. `UHealthComponent` restores the pawn health so the player is not left in
   lobby with 0 HP.

## What This Does Not Do

- no physical teleport to a future lobby room yet;
- no revive/downed state;
- no per-player separate lobby room state;
- no profile/checkpoint commit.

## Verification

Manual PIE check:

- start a run;
- open the console and run `ArenaKillSelf`;
- expect `Arena player died... ReturnedToLobby=true`;
- expect arena state `Phase=Lobby`;
- expect `SetArenaRunStats ... Deaths=... Spendable=0 Earned=0 Committed=0`;
- player pawn should not be hidden/destroyed by generic death behavior.

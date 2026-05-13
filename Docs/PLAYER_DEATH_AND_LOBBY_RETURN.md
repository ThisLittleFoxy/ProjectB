# Player Death And Lobby Return

Status: `GA-06` implemented
Date: 2026-05-08
Depends on: [Threat Death And Rewards](THREAT_DEATH_AND_REWARDS.md)

## Decision

For the current prototype, arena player death returns the run to `Lobby` only
when no teammate can continue the active combat wave, or when death happens
outside the current combat revive window.

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
   - marks the player dead;
   - clears arena runtime rewards;
   - if another arena player is still alive in `Combat`, disables the dead pawn
     until the wave completes and switches the dead player's view target to a
     living teammate;
   - otherwise returns `FArenaRunState` to `Lobby`;
   - resets player ready/alive state for the new lobby state when returning to
     lobby;
   - respawns the existing pawn at the latest arena respawn checkpoint when
     needed.
4. `UHealthComponent` restores the pawn health so the player is not left in
   lobby with 0 HP.

## Respawn Checkpoint

The current checkpoint is the player's actual arena start transform after PIE
spawn offset is applied. `AArenaGameMode` registers it after `RestartPlayer`.
If a `LobbyPlayerSpawn` actor tag exists in the map, that tagged transform is
used instead.

When a combat wave completes, only players that died during that wave are
revived at the checkpoint. Living players keep their current arena position.
The revived player's view target is restored to their own pawn.

Later, checkpoint logic can update the same checkpoint map when a living teammate
reaches the next checkpoint.

## What This Does Not Do

- no separate checkpoint actor/volume system yet;
- no delayed duo revive flow yet;
- no revive/downed state;
- no manual spectator target cycling yet;
- no per-player separate lobby room state;
- no profile/checkpoint commit.

## Verification

Manual PIE check:

- start a run;
- open the console and run `ArenaKillSelf`;
- expect `Arena player died... ReturnedToLobby=true`;
- expect `Arena run returned to lobby. Reason=PlayerDied...`;
- expect `Respawned arena player at checkpoint...`;
- expect arena state `Phase=Lobby`;
- expect `SetArenaRunStats ... Deaths=... Spendable=0 Earned=0 Committed=0`;
- player pawn should not be hidden/destroyed by generic death behavior;
- player pawn should be back at the registered start checkpoint with restored HP.

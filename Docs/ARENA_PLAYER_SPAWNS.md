# Arena Player Spawns

Status: implemented
Date: 2026-05-13

## Goal

Use authored map points to move players between lobby and arena zones without
level travel.

## Map Setup

Place `TargetPoint` actors, or any actor with a usable transform, and assign
these actor tags:

- `LobbyPlayerSpawn` - where players return after death/run reset.
- `ArenaPlayerSpawn` - where players move when the run/wave starts.

For multiple players, place multiple tagged actors. `AArenaGameMode` sorts them
by actor name and assigns players by their `PlayerArray` index, wrapping if
there are fewer spawn points than players.

## Runtime Flow

- `RestartPlayer` registers the player respawn checkpoint. If a
  `LobbyPlayerSpawn` exists, that tagged transform becomes the checkpoint.
- `StartArenaRun` teleports all arena players to `ArenaPlayerSpawn`.
- `StartCombatWaveForDebug` also teleports all arena players to
  `ArenaPlayerSpawn`.
- Returning the run to lobby after player death teleports all arena players to
  `LobbyPlayerSpawn`.

If a tag is missing, the teleport is skipped and a warning is logged. This keeps
older maps playable while making the missing setup obvious in Output Log.

## Relevant Logs

- `Registered arena respawn checkpoint...`
- `Teleported arena player. Context=StartArenaRun...`
- `Teleported arena player. Context=StartCombatWaveForDebug...`
- `Teleported arena player. Context=ReturnArenaRunToLobbyAfterPlayerDeath...`

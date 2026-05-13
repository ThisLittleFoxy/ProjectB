# Arena Wave Director

Status: `Wave-01` implemented, `Wave-02` implemented, manual PIE verification pending
Date: 2026-05-13
Depends on: [Arena Player Spawns](ARENA_PLAYER_SPAWNS.md)

## Goal

Replace the purely debug combat phase with a minimal server-authoritative wave
flow inside `AArenaGameMode`.

## Map Setup

Place one or more spawn actors, for example `TargetPoint`, with actor tag:

- `ThreatSpawnPoint`

Multiple threat spawn points are sorted by actor name and used in a round-robin
pattern.

## GameMode Setup

In `BP_ArenaGameMode`, configure:

- `ThreatClass` - threat actor class to spawn, for example `BP_MonsterDummy`.
- `ThreatsPerWave` - number of threats spawned by the normal ready/start flow.
- `ArenaCountdownDuration` - delay between `Countdown` and `Combat`.
- `ArenaIntermissionDuration` - delay between completed waves.
- `ThreatSpawnPointTag` - default `ThreatSpawnPoint`.

## Runtime Flow

1. All required players ready up.
2. `StartArenaRun` moves players to `ArenaPlayerSpawn` once at run start.
3. Run phase becomes `Countdown`.
4. After `ArenaCountdownDuration`, the server starts the next combat wave.
5. The server spawns `ThreatsPerWave` threats at `ThreatSpawnPoint` actors.
6. `SpawnedThreats` and `AliveThreats` are set from the actual spawn count.
7. Existing GA-05 threat death reporting decrements `AliveThreats`.
8. When `AliveThreats` reaches `0`, phase moves to `Intermission`.
9. Dead players spectate a living teammate until the wave completes.
10. Dead players from the completed wave are revived at their arena checkpoint.
11. Living players are not teleported back to the spawn point between waves.
12. After `ArenaIntermissionDuration`, the run either starts the next
   `Countdown` or returns players to lobby after the final wave.

`StartCombatWaveForDebug` also uses the same spawn path when its
`SpawnedThreats` argument is greater than `0`.

## Failure / Missing Setup

If `ThreatClass` or `ThreatSpawnPoint` is missing, the wave logs a warning and
does not get stuck in `Combat`; it starts as completed and moves to
`Intermission`.

## Relevant Logs

- `Arena run countdown started...`
- `Arena threat spawned...`
- `Arena combat wave started...`
- `Arena intermission advance scheduled...`
- `Arena player spectating teammate...`
- `Arena player revived after completed wave...`
- `Arena next wave countdown started...`
- `Arena run completed after final wave... ReturningToLobby=true`
- `Arena run returned to lobby. Reason=RunCompleted...`
- `Arena threat killed...`

# Arena Threat AI

Status: `TH-01` implemented, manual PIE verification pending

## Goal

`TH-01` adds a minimal server-authoritative rush threat behavior for spawned
arena threats.

The first version intentionally stays small:

- find the nearest alive arena player;
- move toward that player on the server;
- apply contact-range damage on a cooldown;
- replicate threat movement through the spawned threat actor.

## Runtime Path

`AArenaGameMode::SpawnArenaThreatsForWave` now prepares each spawned threat:

1. enables actor replication;
2. enables replicated movement;
3. attaches `UThreatAIComponent` if the threat actor does not already have one.

This means current `BP_MonsterDummy` waves can start moving without reparenting
the Blueprint. Later threat Blueprints can add their own `ThreatAIComponent`
with tuned defaults; the game mode will not add a duplicate.

## Component Defaults

`UThreatAIComponent` is Blueprint-spawnable and configurable:

- `TargetAcquireRadius` - max 2D search radius for alive arena players;
- `MoveSpeed` - direct actor movement speed;
- `StopDistance` - distance where movement stops;
- `AttackRange` - distance required to deal damage;
- `AttackDamage` - damage per hit;
- `AttackInterval` - attack cooldown;
- `bUseSweptMovement` - use swept `SetActorLocation`;
- `bFaceTarget` - rotate the threat toward its target.

The component only ticks on authority. Clients receive movement through actor
replication.

## Current Limitations

- Movement is direct actor movement, not navmesh pathfinding.
- Threat-specific animation, FX, sound, perception, and attack montages are not
  included.
- Threat roster definitions are not included yet.

## PIE Check

1. Ensure `BP_ArenaGameMode` has `ThreatClass` set to `BP_MonsterDummy`.
2. Ensure the map has at least one `ThreatSpawnPoint`.
3. Start an arena run.
4. Expect logs:

```text
Arena threat AI attached...
Arena threat attacked player...
```

5. Let a threat reach a player and verify player HP/death flow uses the existing
   arena death handling.

# Multiplayer Implementation Order

Status: `MP-01` through `MP-05` done
Date: 2026-05-13

## Current Order

1. `MP-01 Replicated Player Presence` - done.
2. `MP-02 Replicated Weapon Presence` - done.
3. `MP-03 Replicated Combat Events` - done.
4. `MP-04 Combat Damage Authority` - done.
5. `MP-05 Player Animation/State Polish` - done.
6. `Wave-01 Arena Wave Director` - implemented.
7. `TH-01 Basic Threat AI` - implemented, manual PIE verification pending.

Spawn/teleport setup notes: [Arena Player Spawns](ARENA_PLAYER_SPAWNS.md).
Wave setup notes: [Arena Wave Director](ARENA_WAVE_DIRECTOR.md).
Threat AI setup notes: [Arena Threat AI](ARENA_THREAT_AI.md).

## MP-02 Boundary

Replicate the visual weapon state other players need to understand:

- loadout weapon classes by slot;
- active weapon slot;
- correct first-person vs third-person attach mesh;
- visible active weapon on remote players.

Out of scope:

- replicated shot traces;
- replicated muzzle/impact FX;
- replicated reload animation;
- authoritative damage rules.

## MP-03 Boundary

Replicate combat events after weapon presence is stable:

- fire events;
- muzzle/impact FX;
- basic remote shot feedback;
- reload/start-stop fire visual state.

Damage authority can remain a separate follow-up if the event plumbing needs to
stay small.

Implementation notes: [Replicated Combat Events](REPLICATED_COMBAT_EVENTS.md).

## MP-04 Boundary

Make damage authoritative and predictable enough for arena gameplay:

- server-side hit validation;
- player-vs-player friendly-fire damage;
- health/death replication expectations;
- no client-local fake damage;
- clear debug logs for rejected/applied damage.

Implementation notes: [Combat Damage Authority](COMBAT_DAMAGE_AUTHORITY.md).

## MP-05 Boundary

Polish the replicated player presentation after core multiplayer combat works:

- host/client character visibility;
- remote movement readability;
- animation setup for normal multiplayer play;
- no invisible remote pawn with only collision;
- no broken T-pose movement in the verified flow.

Implementation notes: [Player Animation State Polish](PLAYER_ANIMATION_STATE_POLISH.md).

## TH-01 Boundary

Add the first playable threat behavior for spawned waves:

- server-only target acquisition;
- direct movement toward alive arena players;
- contact-range damage with cooldown;
- replicated threat actor movement.

Out of scope:

- navmesh/pathfinding;
- threat roster data assets;
- attack animations, FX, and audio;
- advanced perception and avoidance.

Implementation notes: [Arena Threat AI](ARENA_THREAT_AI.md).

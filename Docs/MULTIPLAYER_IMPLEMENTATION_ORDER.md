# Multiplayer Implementation Order

Status: active after `MP-03`
Date: 2026-05-12

## Current Order

1. `MP-01 Replicated Player Presence` - done.
2. `MP-02 Replicated Weapon Presence` - done.
3. `MP-03 Replicated Combat Events` - done.
4. `MP-04 Combat Damage Authority` - current.
5. `MP-05 Player Animation/State Polish` - planned after core multiplayer combat works.

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

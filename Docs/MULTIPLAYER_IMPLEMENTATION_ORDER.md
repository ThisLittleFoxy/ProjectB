# Multiplayer Implementation Order

Status: active after `MP-01`
Date: 2026-05-11

## Current Order

1. `MP-01 Replicated Player Presence` - done.
2. `MP-02 Replicated Weapon Presence` - current.
3. `MP-03 Replicated Combat Events` - next after weapon presence.
4. `MP-04 Combat Damage Authority` - planned after visible combat events.
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

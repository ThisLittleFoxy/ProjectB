# Replicated Combat Events

Status: `MP-03` done
Date: 2026-05-12
Depends on: [Replicated Weapon Presence](REPLICATED_WEAPON_PRESENCE.md)

## Goal

Make combat actions visible and audible to other players after weapon presence is
stable.

## Runtime Contract

`UCombatComponent` owns the network event bridge:

- local owner still plays immediate weapon feedback;
- client fire/reload input is forwarded to the server;
- the server runs its current weapon action;
- server-side weapon fire events multicast cosmetic playback to remote clients;
- multicast fire events are unreliable because they are frequent cosmetic events;
- reload events are reliable because they are sparse state feedback.

`AWeaponBase` owns the local cosmetic playback:

- fire sound;
- dry-fire sound;
- muzzle particle if configured;
- impact particle if configured;
- reload sound if configured.

## Current Asset State

The bridge supports muzzle and impact particles, but the current content does not
need new FX assets for this slice. If a weapon has no particle/sound configured,
the event still replicates and simply has no visible/audio playback for that
asset.

## Out of Scope

- final server-authoritative damage rules;
- ammo replication/prediction polish;
- lag compensation;
- third-person reload/fire montages;
- new authored FX assets.

## Verification

Manual listen-server PIE check passed:

1. Start listen-server PIE with two players.
2. Host fires FastGun; client hears/sees configured weapon feedback.
3. Client fires FastGun; host hears/sees configured weapon feedback.
4. Hold full-auto fire; the other player receives repeated fire feedback.
5. Reload on one player; the event path is implemented, but current content has
   no reload sound asset.
6. Confirm damage/death behavior is not treated as MP-03 acceptance criteria.

## Follow-Up Closed

Friendly-fire and server-authoritative damage were completed in
[Combat Damage Authority](COMBAT_DAMAGE_AUTHORITY.md).

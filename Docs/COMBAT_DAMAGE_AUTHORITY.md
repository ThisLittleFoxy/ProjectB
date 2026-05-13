# Combat Damage Authority

Status: `MP-04` done
Date: 2026-05-13
Depends on: [Replicated Combat Events](REPLICATED_COMBAT_EVENTS.md)

## Goal

Make combat damage server-authoritative while keeping MP-03 cosmetic fire/reload
events unchanged.

## Runtime Contract

`AWeaponBase` owns weapon hit validation:

- only authority applies real weapon damage;
- client-side weapon fire can still play local cosmetics;
- shot traces keep the existing `Visibility` trace and add a pawn object trace
  fallback for player capsules;
- if both traces hit, the closest blocking result wins;
- player targets respect friendly-fire rules unless the weapon explicitly
  bypasses friendly-fire checks.

`UHealthComponent` is the final health mutation guard:

- client-local damage mutation is rejected;
- player-target friendly-fire damage is rejected when
  `bAcceptFriendlyFireDamage` is disabled on the target;
- weapon-level `bBypassFriendlyFireRules` can override the target friendly-fire
  flag for special sources like future fire/explosive weapons;
- arena death/reward reporting still flows through the existing GA-05/GA-06
  paths.

## Current Friendly-Fire Rules

- Character flag: `UHealthComponent::bAcceptFriendlyFireDamage`.
- Weapon flag: `AWeaponBase::bBypassFriendlyFireRules`.
- Normal hitscan self-hit is still ignored by owner/instigator trace filtering.
  Self-damage should come from a deliberate damage source such as a debug command,
  explosion, fire volume, or future area weapon.

## Verification

Manual listen-server PIE verification passed:

1. Build `ProjectEditor Win64 Development -NoLiveCoding`.
2. Start listen-server PIE with two players.
3. Shoot the other player from host and from client.
4. Confirm server-authoritative damage/death behavior.
5. Confirm death returns the player through the checkpoint respawn flow.
6. Confirm friendly-fire rules behave as configured by character/weapon flags.

## Out of Scope

- health replication UI polish;
- lag compensation;
- projectile/explosion damage sources;
- player animation/state polish after damage/death.

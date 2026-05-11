# Server-Authoritative Purchase

Status: `GA-04` implemented
Date: 2026-05-08
Depends on: [Arena Runtime Currency](ARENA_RUNTIME_CURRENCY.md), [Armory Access Model](ARMORY_ACCESS_MODEL.md)

## Decision

Arena weapon purchases go through the owning `AMainPlayerController` and are
validated on the server.

The client may open shop UI locally, but it no longer owns the arena purchase
decision.

## Request Path

1. `UWeaponShopWidgetBase::PurchaseWeapon(...)`
2. `AMainPlayerController::RequestArenaPurchaseWeapon(...)`
3. `ServerRequestArenaPurchaseWeapon(...)`
4. `HandleArenaPurchaseWeapon(...)` on authority
5. `ClientArenaPurchaseWeaponResult(...)` updates the owning client's local
   armory item list and refreshes UI.

## Server Validation

The server validates:

- shop terminal exists;
- requested weapon exists in that terminal's offers;
- price is resolved server-side from the terminal offer or weapon CDO;
- player does not already own the weapon;
- player has inventory space;
- `AArenaPlayerState::SpendableCurrency` is enough.

On success:

- server spends `AArenaPlayerState::SpendableCurrency`;
- server grants the weapon into `UPlayerArmoryComponent`;
- owning client receives the result and mirrors the weapon item locally for UI
  refresh;
- UI money is read from `AArenaPlayerState::SpendableCurrency` in arena mode,
  not copied into `UPlayerArmoryComponent`.

## What This Does Not Do

- inventory/loadout replication is still not a complete replicated state model;
- shop terminal distance/line-of-sight validation is not implemented yet;
- purchase failure UI text is not implemented yet;
- profile persistence is not implemented yet.

## Verification

Manual PIE check:

1. Give the player enough arena spendable currency, for example through a debug
   setup or future reward flow.
2. Open a shop terminal.
3. Buy an offered weapon.
4. Expected server log:
   `Arena purchase accepted. Player=... Weapon=... Price=... Remaining=...`
5. Expected owning client log:
   `ClientArenaPurchaseWeaponResult ... Success=true Remaining=...`

Rejected purchases should log one of:

- `Arena purchase rejected: invalid shop offer...`
- `Arena purchase price mismatch...`
- `Arena purchase rejected...`

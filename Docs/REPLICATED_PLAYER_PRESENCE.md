# Replicated Player Presence

Status: `MP-01` implemented
Date: 2026-05-08
Depends on: [Listen-server Readiness](LISTEN_SERVER_READINESS.md)

## Decision

For the first multiplayer presence slice, player pawn visibility and movement
are configured at runtime when `AMainPlayerController` possesses a pawn.

This keeps the slice independent from Blueprint asset churn while we are still
using a Blueprint character.

## Runtime Behavior

On possess, `AMainPlayerController`:

- enables pawn replication;
- enables movement replication;
- enables character movement component replication when present;
- treats meshes named like `FirstPerson` or `Arms` as owner-only;
- keeps other skeletal meshes visible to other players, but hidden from their
  owning player with `OwnerNoSee`;
- reapplies the same mesh visibility rules on the owning client through
  `AcknowledgePossession`.

## What This Does Not Do

- no replicated weapon actor/loadout visuals yet;
- no replicated firing/reload/combat prediction;
- no Steam/session layer;
- no full third-person animation setup.

## Verification

Manual PIE check:

1. Start listen-server PIE with two players.
2. Look for:
   `Configured pawn network presence. Controller=... Pawn=... Meshes=...`
3. Move host and client.
4. Host should see the client pawn move.
5. Client should see the host pawn move.
6. First-person arms should remain owner-only.

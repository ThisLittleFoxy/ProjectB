# Replicated Weapon Presence

Status: `MP-02` done
Date: 2026-05-12
Depends on: [Replicated Player Presence](REPLICATED_PLAYER_PRESENCE.md)

## Goal

Make each player see the other player's active weapon in listen-server PIE.

## Runtime Contract

`UCombatComponent` is the replicated source for weapon presence:

- server writes weapon classes per loadout slot;
- server writes the active weapon slot;
- clients spawn local visual weapon actors from the replicated classes;
- local owner attaches weapons to the first-person mesh;
- remote viewers attach weapons to the third-person character mesh.

This keeps `MP-02` focused on presence, not full combat prediction.

## Verification

Manual listen-server PIE check passed:

1. Start listen-server PIE with two players.
2. Verify host sees the client pawn and the client's active weapon.
3. Verify client sees the host pawn and the host's active weapon.
4. Cycle weapon on one player.
5. Verify the other player sees the active weapon update.
6. Buy weapons on the client, assign them to loadout slots, and verify the host sees the active weapon.
7. Buy weapons on the host, assign them to loadout slots, and verify the client sees the active weapon.

Out of scope for this check:

- muzzle flash replication;
- hit replication;
- reload replication;
- final third-person animation polish.

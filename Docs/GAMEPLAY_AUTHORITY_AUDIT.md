# Gameplay Authority Audit

Статус: результат `NF-03`
Дата: 2026-05-06
Source of truth: [NOTION_GDD.md](NOTION_GDD.md)
Опирается на: [NETWORK_FOUNDATION_ORDER.md](NETWORK_FOUNDATION_ORDER.md), [LISTEN_SERVER_READINESS.md](LISTEN_SERVER_READINESS.md), [PROFILE_SAVE_STRATEGY.md](PROFILE_SAVE_STRATEGY.md)

## Цель

Зафиксировать, какие gameplay decisions должны быть server-authoritative перед `Duo`, а какие части остаются локальным input/UI.

Этот audit не реализует Steam, session layer, wave spawning, shop rewrite или full combat replication. Результат - список маленьких code tasks, которые можно брать по одному.

## Текущий снимок

- `AArenaGameMode` уже владеет debug phase flow и публикует `FArenaRunState`.
- `AArenaGameState` уже реплицирует `ArenaRunState`.
- `AArenaPlayerState` уже реплицирует `ArenaRunStats`.
- `NF-02` подтвердил, что listen-server клиент получает `Lobby`, `Countdown`, `Combat`, `Intermission`, `Result` через `OnRep_ArenaRunState`.
- Старые gameplay systems всё ещё single-player/local: `AMainPlayerController`, `UCombatComponent`, `AWeaponBase`, `UHealthComponent`, `UCurrencyComponent`, `UPlayerArmoryComponent`, shop/inventory widgets.
- В старых gameplay systems сейчас нет `Server`, `Client`, `NetMulticast` RPC. Репликация есть только в новых arena `GameState` / `PlayerState`.
- `UProjectSaveSubsystem` остаётся legacy snapshot save/load. Arena profile должен быть отдельным слоем.

## Authority Rules

Сервер должен владеть:

- фазой run;
- ready/start run;
- стартом и завершением волн;
- спавном угроз;
- смертью угроз и kill credit;
- смертью игроков и fail/success условиями;
- runtime rewards;
- checkpoint commit;
- shop purchase validation;
- применением loadout к pawn во время arena run;
- итоговыми run result данными для future profile commit.

Клиент может владеть:

- input intent;
- camera;
- локальным aim/recoil feel;
- локальным UI;
- hover/drag/drop preview;
- audio/visual feedback;
- запросами к серверу: ready, start, fire, reload, purchase, change loadout.

UI и `PlayerController` не должны становиться владельцами arena rules. Их роль - собрать input и отправить request.

## Match Flow

### Ready / Start Run

Current owner:

- полноценного ready/start flow нет;
- текущий debug flow живёт в `AArenaGameMode` и запускается вручную/opt-in.

Target owner:

- `AArenaGameMode`.

Client role:

- игрок нажимает ready/start;
- локальный `PlayerController` отправляет server request.

Replicated state:

- ready state в `AArenaPlayerState` или отдельной replicated player entry;
- phase/roster/run mode в `AArenaGameState`.

Gap:

- нет RPC/request path от клиента к server-authoritative arena runtime;
- нет replicated ready flag.

Next task:

- `GA-01`: добавить минимальный ready/start run request path.

### Phase / Wave Transitions

Current owner:

- `AArenaGameMode` уже умеет менять phases через debug methods.

Target owner:

- `AArenaGameMode`.

Client role:

- только отображение phase и отправка разрешённых requests.

Replicated state:

- `FArenaRunState` в `AArenaGameState`.

Gap:

- debug methods ещё не являются gameplay rules;
- нет условий перехода `Lobby -> Countdown -> Combat -> Intermission -> Result`.

Next task:

- после `GA-01` заменить debug-only start на явные gameplay methods: `StartRun`, `StartWave`, `CompleteWave`, `FailRun`, `CompleteRun`.

### Threat Spawn / Alive Threat Count

Current owner:

- угрозы/wave spawning ещё не реализованы в arena runtime.

Target owner:

- `AArenaGameMode` или отдельный server-only `ArenaWaveDirector`, вызываемый из `AArenaGameMode`.

Client role:

- отображать replicated threats/phase counters.

Replicated state:

- `WaveState.CurrentWave`, `AliveThreats`, `SpawnedThreats`;
- сами threat actors должны spawn на сервере и реплицироваться при необходимости.

Gap:

- нет authoritative spawn list;
- нет binding между смертью угрозы и `AliveThreats`.

Next task:

- `GA-05`: добавить server-side threat death reporting и обновление wave counters.

## Combat And Death

### Fire / Trace / Damage

Current owner:

- `AMainPlayerController` вызывает `UCombatComponent::StartFire`.
- `UCombatComponent` вызывает `AWeaponBase::StartFire`.
- `AWeaponBase::FireOnce` делает trace, damage и ammo consume локально.

Target owner:

- сервер должен подтверждать fire/damage/ammo для co-op.

Client role:

- input intent, local camera recoil, local FX/prediction later.

Replicated state:

- ammo/current weapon/loadout result;
- health/death state targets;
- damage result events if needed for UI.

Gap:

- fire path сейчас не server-authoritative;
- ammo не replicated;
- damage может быть рассчитан клиентом.

Next task:

- не первым шагом. Сначала стоит сделать ready/start and economy boundaries, потом отдельный combat replication slice.

### Threat Death / Kill Credit

Current owner:

- `UHealthComponent` реагирует на damage и может выдать currency через `GrantDeathCurrencyReward`.

Target owner:

- смерть угрозы должна попадать в `AArenaGameMode`;
- kill credit и rewards должны обновлять `AArenaPlayerState`, а не напрямую локальный wallet.

Client role:

- показать kill/reward feedback после replicated result.

Replicated state:

- `ArenaRunStats.Kills`;
- `ArenaRunStats.EarnedCurrency`;
- `WaveState.AliveThreats`.

Gap:

- current `HealthComponent` смешивает смерть и currency reward;
- в arena flow reward должен учитывать wave commit rule.

Next task:

- `GA-05`: добавить server-side death event bridge без полной переделки combat.

### Player Death / Run Fail

Current owner:

- `UHealthComponent::HandleDeath` может hide/destroy owner.

Target owner:

- `AArenaGameMode` решает, что значит смерть игрока:
  - в `Solo` - fail текущего run;
  - в `Duo` - fail только когда оба игрока мертвы;
  - disconnect/host leave - отдельное правило.

Client role:

- отображать death/downed/fail UI.

Replicated state:

- player alive/dead/downed flag;
- run result;
- phase/result in `AArenaGameState`.

Gap:

- нет arena-specific player life state;
- auto destroy/hide может быть неподходящим для player pawn в arena.

Next task:

- `GA-06`: определить player death handling для arena pawn и fail conditions.

## Economy, Shop, Loadout

### Runtime Currency / Rewards

Current owner:

- `UCurrencyComponent` хранит local wallet на pawn;
- `UPlayerArmoryComponent` кэширует currency и слушает pawn currency changes.

Target owner:

- server-authoritative runtime currency в `AArenaPlayerState` или server-owned player runtime component;
- permanent currency только в future profile layer.

Client role:

- отображать replicated wallet;
- отправлять purchase/request actions.

Replicated state:

- earned currency;
- committed currency;
- optional current spendable runtime currency.

Gap:

- `UCurrencyComponent` не replicated;
- wallet сейчас связан с old snapshot save/load flow.

Next task:

- `GA-03`: перенести arena runtime currency в `AArenaPlayerState` API.

### Shop Open

Current owner:

- `UInteractionComponent` делает local trace;
- `AWeaponShopTerminal::OnInteract` вызывает `AMainPlayerController::OpenWeaponShop`;
- открытие UI локальное.

Target owner:

- открытие UI может оставаться client-local;
- сервер должен валидировать gameplay action purchase.

Client role:

- открыть UI;
- показать offers;
- отправить purchase request.

Replicated state:

- phase, wallet, owned weapons/loadout.

Gap:

- shop access должен определяться наличием terminal в зоне, а не phase state;
- purchase mutation происходит локально в armory component.

Next task:

- `GA-02`: зафиксировать armory access model;
- `GA-04`: server-authoritative purchase request.

### Purchase Weapon

Current owner:

- `UWeaponShopWidgetBase::PurchaseWeapon` вызывает `UPlayerArmoryComponent::PurchaseWeapon`;
- `UPlayerArmoryComponent` проверяет ownership, inventory space, currency and spends locally.

Target owner:

- сервер валидирует:
  - phase allows shop;
  - player owns/does not own weapon as expected;
  - price;
  - runtime/permanent currency;
  - inventory capacity;
  - current match rules.

Client role:

- sends request and shows pending/failed/success result.

Replicated state:

- wallet;
- owned weapons;
- inventory/loadout state if it affects current run.

Gap:

- purchase is currently a local UI/component mutation;
- no server validation or replicated result.

Next task:

- `GA-04`: add `ServerRequestPurchaseWeapon` and authoritative result update.

### Loadout Changes

Current owner:

- inventory widgets call `UPlayerArmoryComponent` directly;
- armory component applies loadout to `UCombatComponent` immediately.

Target owner:

- outside run / profile screen: future profile subsystem owns persistent loadout;
- during arena intermission: server validates and applies current run loadout.

Client role:

- drag/drop preview can stay local;
- committed changes become server requests.

Replicated state:

- selected loadout;
- current weapon slot;
- spawned/equipped weapon actors as needed.

Gap:

- drag/drop and loadout assignment currently mutate local armory state;
- no phase validation.

Next task:

- `GA-02`: add phase/read-only gating first;
- later `GA-04` or separate `GA-07`: server-authoritative loadout commit.

### Checkpoint Commit / Profile

Current owner:

- not implemented for arena;
- `UProjectSaveSubsystem` stores old snapshot data.

Target owner:

- `AArenaGameMode` produces authoritative run/wave result;
- future `UProjectProfileSubsystem` applies profile commit;
- `UProjectSaveSubsystem` stays out of arena progression.

Client role:

- show result/commit UI.

Replicated state:

- `ArenaRunStats.EarnedCurrency`;
- `ArenaRunStats.CommittedCurrency`;
- run result summary.

Gap:

- profile subsystem does not exist yet;
- `UProjectProfileSaveGame` is currently minimal.

Next task:

- not before core runtime authority. Keep as future profile implementation after `GA-03`/`GA-05`.

## Save / Load Boundary

Current owner:

- `AMainPlayerController` and `UProjectGameViewportClient` call `UProjectSaveSubsystem`.

Target owner:

- legacy save/load remains for old prototype;
- arena run should not use quick/manual snapshot saves as checkpoint.

Client role:

- menu commands only where allowed.

Gap:

- arena-specific UI/menu should gate or disable snapshot save/load during active run.

Next task:

- `GA-00`: disable or hide quick/manual save/load during active arena run if the arena map uses old menus.

## Suggested Small Task Queue

Recommended order:

1. `GA-01 Ready/Start Run Request` - done, verified in [Ready / Start Run Request](READY_START_RUN_REQUEST.md)
   - add client-to-server request path;
   - add replicated ready flag;
   - move run start from debug/manual call to server validation.

2. `GA-02 Armory Access Model` - done in [Armory Access Model](ARMORY_ACCESS_MODEL.md)
   - shop access is location/content driven, not phase-gated;
   - inventory can open in any arena phase;
   - loadout editing is allowed during active waves.

3. `GA-03 Arena Runtime Currency In PlayerState` - done in [Arena Runtime Currency](ARENA_RUNTIME_CURRENCY.md)
   - add server-owned runtime currency API to `AArenaPlayerState`;
   - replicate earned/committed/spendable values;
   - stop treating `UCurrencyComponent` as arena source of truth.

4. `GA-04 Server-Authoritative Purchase` - done in [Server-Authoritative Purchase](SERVER_AUTHORITATIVE_PURCHASE.md)
   - add purchase request;
   - validate phase, price, ownership, capacity, currency;
   - replicate result to owning client.

5. `GA-05 Threat Death And Rewards` - done in [Threat Death And Rewards](THREAT_DEATH_AND_REWARDS.md)
   - bridge threat death into `AArenaGameMode`;
   - update kill credit, alive threat count and earned rewards on server.

6. `GA-06 Player Death / Lobby Return` - done in [Player Death And Lobby Return](PLAYER_DEATH_AND_LOBBY_RETURN.md)
   - add arena player alive/dead state;
   - return the run to lobby on player death;
   - clear arena runtime rewards on player death;
   - avoid generic auto-destroy behavior for arena player death unless explicitly wanted.

## Current Recommendation

Next implementation task should be selected after `GA-06` implementation.

Reason:

- `GA-01` already verified the first client-to-server gameplay request path;
- `GA-02` now fixes the access model for shop/inventory/loadout;
- `GA-03` moved arena runtime currency into replicated `AArenaPlayerState` data/API;
- `GA-04` moved arena purchase validation to the server-side request path;
- `GA-05` moved death rewards and wave threat counts into `AArenaGameMode`;
- `GA-06` returns the run to lobby on player death and clears arena runtime gains;
- `MP-01 Replicated Player Presence` remains the best visual/network confidence slice if we want both players visible.

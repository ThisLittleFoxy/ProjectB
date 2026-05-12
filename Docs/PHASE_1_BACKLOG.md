# Phase 1 Backlog - Design & Scope

Статус: рабочий список мелких задач
Связанный GDD: [ProjectB - Arena Co-op v1](NOTION_GDD.md)

## Цель фазы

Зафиксировать продуктовые и технические решения для `Arena Co-op v1` до начала крупной реализации. Эта фаза не должна превращаться в разработку всего коопа, Steam, волн и UI одновременно.

## Правила дробления

- Один task должен давать один проверяемый результат.
- Документационные решения отделяются от кода.
- Первый кодовый slice должен быть solo-only или local-only, если это уменьшает риск.
- Steam OSS, invite flow, leaderboards и полноценный duo runtime не смешиваются в один шаг.
- Каждый следующий task должен либо уменьшать неопределённость, либо добавлять маленький устойчивый слой.

## Source of Truth

- Актуальный GDD: [NOTION_GDD.md](NOTION_GDD.md)
- Старый GDD: [GDD.md](GDD.md), только historical/context
- Старый список первого прототипа: [../FIRST_PROTOTYPE_TASKS.md](../FIRST_PROTOTYPE_TASKS.md), только historical/context

## Tasks

### P1-00. Зафиксировать документы

Статус: done

Результат:

- `NOTION_GDD.md` помечен как основной source of truth;
- старые документы помечены как legacy/context;
- создан этот Phase 1 backlog.

### P1-01. Провести technical inventory текущего фундамента

Статус: done

Задача:

- сопоставить текущие системы проекта с блоками из GDD;
- отдельно отметить, что можно переиспользовать, что нужно адаптировать, а что отсутствует.

Ожидаемый результат:

- таблица `Current system -> Arena Co-op v1 block -> decision`;
- список первых безопасных code slices.

Результат:

- [Technical Inventory - Arena Co-op v1](TECHNICAL_INVENTORY.md)

Кандидаты для анализа:

- `MainPlayerController`
- combat/weapon components
- inventory/loadout
- weapon shop
- currency
- save subsystem
- menu/viewport UI
- current GameMode/GameState setup in Blueprint/config

### P1-02. Зафиксировать минимальную архитектуру Arena runtime

Статус: done

Задача:

- описать минимальные ответственности `ArenaGameMode`, `ArenaGameState`, `ArenaPlayerState`;
- отделить runtime state от persistent profile;
- решить, какие данные должны быть replicated сразу, а какие можно оставить local-only до network phase.

Ожидаемый результат:

- короткий architecture note;
- список C++ классов для первого skeleton task.

Результат:

- [Arena Runtime Architecture](ARENA_RUNTIME_ARCHITECTURE.md)

### P1-03. Определить первый code slice

Статус: done

Задача:

- выбрать один маленький стартовый implementation task после P1-01 и P1-02.

Предпочтительный вариант:

- solo-only `ArenaGameMode` skeleton;
- базовые enum/struct для phase flow;
- без Steam OSS;
- без spawn waves;
- без UI переписывания.

Ожидаемый результат:

- один конкретный task, который можно сделать и проверить компиляцией.

Результат:

- [First Implementation Slice - Arena Runtime Skeleton](FIRST_IMPLEMENTATION_SLICE.md)

### P1-04. Решить порядок network foundation

Статус: done

Задача:

- определить, что идёт раньше: local listen-server runtime или Steam session/invite;
- зафиксировать, какие части должны быть server-authoritative с первого шага.

Ожидаемый результат:

- короткое решение в GDD или отдельной technical note;
- запрет на смешивание Steam setup и arena rules в одном task.

Результат:

- [Network Foundation Order](NETWORK_FOUNDATION_ORDER.md)

### P1-05. Решить связь save/profile с Arena progression

Статус: done

Задача:

- понять, как текущий `ProjectSaveSubsystem` соотносится с `PlayerPersistentProfile`;
- решить, мигрируем ли текущие save slots или добавляем отдельный profile слой.

Ожидаемый результат:

- решение по persistent profile;
- список полей профиля для первой версии;
- отдельный future task для реализации.

Результат:

- [Profile Save Strategy](PROFILE_SAVE_STRATEGY.md)

## Следующий непосредственный шаг

Взять следующий маленький implementation task из [Gameplay Authority Audit](GAMEPLAY_AUTHORITY_AUDIT.md).

Текущий task: выбрать следующий шаг после `MP-01 Replicated Player Presence`.

Контекст:

- выбранный code slice [First Implementation Slice - Arena Runtime Skeleton](FIRST_IMPLEMENTATION_SLICE.md) уже реализован и собран;
- порядок network foundation зафиксирован в [Network Foundation Order](NETWORK_FOUNDATION_ORDER.md);
- save/profile strategy зафиксирована в [Profile Save Strategy](PROFILE_SAVE_STRATEGY.md);
- local arena runtime phase flow реализован и описан в [Local Arena Runtime Flow](LOCAL_ARENA_RUNTIME_FLOW.md);
- listen-server readiness проверен и зафиксирован в [Listen-server Readiness](LISTEN_SERVER_READINESS.md);
- gameplay authority audit зафиксирован в [Gameplay Authority Audit](GAMEPLAY_AUTHORITY_AUDIT.md);
- перед session/Steam работой нужно завести повторяемый client-to-server request pattern для gameplay decisions.

Текущий результат:

- `GA-01 Ready/Start Run Request` реализован, проверен и описан в [Ready / Start Run Request](READY_START_RUN_REQUEST.md).
- `GA-02 Armory Access Model` зафиксирован в [Armory Access Model](ARMORY_ACCESS_MODEL.md).
- `GA-03 Arena Runtime Currency In PlayerState` реализован и описан в [Arena Runtime Currency](ARENA_RUNTIME_CURRENCY.md).
- `GA-04 Server-Authoritative Purchase` реализован и описан в [Server-Authoritative Purchase](SERVER_AUTHORITATIVE_PURCHASE.md).
- `GA-05 Threat Death And Rewards` реализован и описан в [Threat Death And Rewards](THREAT_DEATH_AND_REWARDS.md).
- `GA-06 Player Death / Lobby Return` реализован и описан в [Player Death And Lobby Return](PLAYER_DEATH_AND_LOBBY_RETURN.md).
- `MP-01 Replicated Player Presence` реализован и описан в [Replicated Player Presence](REPLICATED_PLAYER_PRESENCE.md).

Текущий task:
- `MP-02 Replicated Weapon Presence` - выбран как следующий multiplayer slice.

Следующий task после MP-02:
- `MP-03 Replicated Combat Events`, если weapon presence работает стабильно;
- `Wave-01 Arena Wave Director`, если решим временно переключиться на wave spawning.

# Armory Access Model

Статус: решение `GA-02`
Дата: 2026-05-06
Опирается на: [GAMEPLAY_AUTHORITY_AUDIT.md](GAMEPLAY_AUTHORITY_AUDIT.md), [READY_START_RUN_REQUEST.md](READY_START_RUN_REQUEST.md)

## Решение

Shop, inventory и loadout не блокируются по `EArenaPhase`.

Причина: будущая структура локаций разделяет подготовку и арену физически:

- `Lobby / Armory room` - место подготовки, где стоят shop/loadout terminals;
- `Arena room` - место боя, где shop terminal просто отсутствует.

Значит, доступ к shop должен определяться не фазой run, а наличием interactable terminal в текущей зоне. Если на арене нет магазина, игрок не сможет открыть shop естественным образом.

## Loadout During Waves

Loadout можно менять во время активной волны.

Продуктовая логика:

- игрок может открыть inventory в любой фазе;
- игрок может переключить/пересобрать loadout в любой фазе;
- это поддерживает свободу выбора оружия во время длинных волн;
- server-authoritative validation может быть добавлена позже, но не должна запрещать саму идею смены loadout во время боя.

## Current Code State

После корректировки `GA-02`:

- `OpenInventory` не блокируется по arena phase;
- `OpenWeaponShop` не блокируется по arena phase;
- `UWeaponShopWidgetBase::PurchaseWeapon` не блокируется по arena phase;
- `UPlayerInventoryWidgetBase` loadout/grid mutations не блокируются по arena phase;
- старый single-player prototype остаётся в прежней модели.

## Что не входит

- server-authoritative purchase;
- server-authoritative loadout commit;
- replicated inventory/loadout state;
- физическая структура будущих lobby/arena помещений;
- удаление shop terminal с будущей arena room;
- Steam/session.

## Будущие правила

### Shop

Shop доступен, если:

- игрок взаимодействует с shop terminal;
- terminal существует в текущей зоне;
- будущий server-side purchase validation подтверждает цену, валюту и ownership.

Shop недоступен на арене не через phase gate, а через level/content layout: на арене не размещается shop terminal.

### Inventory / Loadout

Inventory доступен всегда.

Loadout editing допускается во время run. Будущий authority слой должен валидировать и реплицировать результат, но не запрещать смену оружия только потому, что идёт волна.

## Проверка

1. В `Lobby` открыть inventory/shop.
2. Нажать ready и перейти в `Countdown`.
3. Во время `Countdown` открыть inventory.
4. Во время `Countdown` попробовать loadout/grid actions.
5. Если shop terminal всё ещё стоит на тестовой карте, shop тоже может открыться. Это допустимо для текущей test map.

Ожидаемо:

- inventory открывается в любой фазе;
- loadout/grid actions не блокируются phase guard'ом;
- shop открывается только там, где физически есть terminal;
- нет log строк `blocked by arena phase`.

## Следующий шаг

После этой корректировки выбор следующий:

- `MP-01 Replicated Player Presence`, если хотим сначала увидеть второго игрока и его движение;
- `GA-03 Arena Runtime Currency In PlayerState`, если продолжаем economy/authority цепочку.


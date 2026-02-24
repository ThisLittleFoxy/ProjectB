# Sniper Scope PiP Setup (UE5 BP)

This project now exposes helper APIs to wire the scope overlay quickly:

- `CombatComponent.IsScopeOverlayActive()`
- `CrosshairWidgetBase.ShouldShowSniperScopeOverlay()`
- `CrosshairWidgetBase.GetCurrentWeaponTypeTag()`

## 1) Create assets

1. Create `RT_SniperScope_1024` (`RenderTarget2D`, `1024x1024`).
2. Create UI material `MI_UI_SniperScope`:
   - Domain: `User Interface`
   - Inputs:
     - `ScopeTex` (Texture parameter) -> `RT_SniperScope_1024`
     - `ReticleTex` (optional texture)
   - Mask:
     - Inside circle -> show `ScopeTex`
     - Outside -> black vignette
   - Defaults:
     - `Radius=0.46`
     - `SoftEdge=0.01`

## 2) Character BP (`BP_Character`)

1. Add `SceneCaptureComponent2D` named `SC_SniperScope`.
2. Parent to first-person camera.
3. Set:
   - `Texture Target = RT_SniperScope_1024`
   - `FOV Angle = 12.0`
   - `Capture Every Frame = false`
   - `Capture on Movement = false`
4. Add bool variable `bSniperScopeActive`.
5. In Tick (or timer function):
   - `CombatComponent` -> `IsScopeOverlayActive()`
   - On state change:
     - `true`: enable `SC_SniperScope`, set capture flags true
     - `false`: disable capture flags and hide component

## 3) Smooth ADS FOV (0.12s)

1. In character's `CombatComponent` instance set `bEnableScopeFov = false`.
2. In `BP_Character`:
   - `DefaultFOV` from camera at BeginPlay
   - `ScopedFOV = 40.0`
   - Timeline `TL_ScopeFOV` duration `0.12`, track `Alpha 0..1`
   - Update: `SetFieldOfView(Lerp(DefaultFOV, ScopedFOV, Alpha))`
3. Trigger timeline play/reverse using scope active state transitions.

## 4) HUD (`WBP_HUD` based on `CrosshairWidgetBase`)

1. Add full-screen `Image` named `IMG_SniperScope`.
2. Set `Brush` to material `MI_UI_SniperScope`.
3. Bind `Visibility` to `ShouldShowSniperScopeOverlay()`:
   - true -> `Visible`
   - false -> `Collapsed`
4. Optionally add fade animation `Anim_ScopeFade` for 0.12s.

## 5) Weapon tags

1. In `BP_Weapon_Sniper` set `WeaponTypeTag = Weapon.Type.Sniper`.
2. Other weapons use their own tags.
3. If needed, change `CombatComponent.ScopeOverlayWeaponTypeTag` from default sniper tag.

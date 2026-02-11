#include "Combat/WeaponRifle.h"

AWeaponRifle::AWeaponRifle()
{
    Damage = 18.0f;
    RoundsPerMinute = 750.0f;
    MaxRange = 25000.0f;
    SpreadAngleDeg = 0.75f;
    RecoilPitch = 0.8f;
    RecoilYaw = 0.3f;
}

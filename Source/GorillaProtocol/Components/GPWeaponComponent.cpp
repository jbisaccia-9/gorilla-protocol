#include "Components/GPWeaponComponent.h"

#include "Components/GPHealthComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "TimerManager.h"

UGPWeaponComponent::UGPWeaponComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UGPWeaponComponent::BeginPlay()
{
    Super::BeginPlay();
    MagazineAmmo = Tuning.MagazineSize;
    ReserveAmmo = StartingReserveAmmo;
    OnAmmoChanged.Broadcast(MagazineAmmo, ReserveAmmo);
}

void UGPWeaponComponent::ConfigureWeapon(const FGPWeaponTuning& NewTuning, int32 NewReserveAmmo)
{
    Tuning = NewTuning;
    StartingReserveAmmo = FMath::Max(0, NewReserveAmmo);
    if (HasBegunPlay())
    {
        MagazineAmmo = Tuning.MagazineSize;
        ReserveAmmo = StartingReserveAmmo;
        OnAmmoChanged.Broadcast(MagazineAmmo, ReserveAmmo);
    }
}

void UGPWeaponComponent::StartFire()
{
    if (FireSingleShot())
    {
        GetWorld()->GetTimerManager().SetTimer(FireTimer, this, &UGPWeaponComponent::HandleAutoFire,
            Tuning.FireInterval, true, Tuning.FireInterval);
    }
}

void UGPWeaponComponent::HandleAutoFire()
{
    FireSingleShot();
}

void UGPWeaponComponent::StopFire()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(FireTimer);
    }
}

bool UGPWeaponComponent::FireSingleShot()
{
    if (!GetWorld() || bReloading || MagazineAmmo <= 0)
    {
        StopFire();
        if (MagazineAmmo <= 0) Reload();
        return false;
    }

    const double Now = GetWorld()->GetTimeSeconds();
    if (Now - LastShotTime + KINDA_SMALL_NUMBER < Tuning.FireInterval)
    {
        return false;
    }

    FVector ViewLocation;
    FRotator ViewRotation;
    if (!ResolveViewpoint(ViewLocation, ViewRotation))
    {
        return false;
    }

    LastShotTime = Now;
    --MagazineAmmo;
    OnAmmoChanged.Broadcast(MagazineAmmo, ReserveAmmo);

    const float SpreadRadians = FMath::DegreesToRadians(bAiming ? Tuning.AimSpreadDegrees : Tuning.HipSpreadDegrees);
    const FVector ShotDirection = FMath::VRandCone(ViewRotation.Vector(), SpreadRadians);
    const FVector TraceEnd = ViewLocation + ShotDirection * Tuning.Range;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GPWeaponTrace), true, GetOwner());
    QueryParams.bReturnPhysicalMaterial = true;

    FHitResult Hit;
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams);
    if (bHit)
    {
        const bool bHeadshot = Hit.BoneName.ToString().Contains(TEXT("head"), ESearchCase::IgnoreCase) ||
            (Hit.GetComponent() && Hit.GetComponent()->ComponentHasTag(TEXT("HitZone.Head")));
        const float AppliedDamage = Tuning.Damage * (bHeadshot ? Tuning.HeadshotMultiplier : 1.0f);
        if (UGPHealthComponent* Health = Hit.GetActor() ? Hit.GetActor()->FindComponentByClass<UGPHealthComponent>() : nullptr)
        {
            Health->ReceiveDamage(AppliedDamage, GetOwner());
        }
    }

    if (APawn* PawnOwner = Cast<APawn>(GetOwner()))
    {
        UAISense_Hearing::ReportNoiseEvent(GetWorld(), PawnOwner->GetActorLocation(), 1.0f, PawnOwner, 2200.0f,
            TEXT("Weapon.Gunshot"));
    }
    if (FireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, FireSound, ViewLocation);
    }
    BP_OnWeaponFired(Hit, bHit);
    return true;
}

void UGPWeaponComponent::Reload()
{
    if (!GetWorld() || bReloading || MagazineAmmo >= Tuning.MagazineSize || ReserveAmmo <= 0)
    {
        return;
    }

    StopFire();
    bReloading = true;
    GetWorld()->GetTimerManager().SetTimer(ReloadTimer, this, &UGPWeaponComponent::CompleteReload,
        Tuning.ReloadDuration, false);
}

void UGPWeaponComponent::CompleteReload()
{
    const int32 Needed = Tuning.MagazineSize - MagazineAmmo;
    const int32 Loaded = FMath::Min(Needed, ReserveAmmo);
    MagazineAmmo += Loaded;
    ReserveAmmo -= Loaded;
    bReloading = false;
    OnAmmoChanged.Broadcast(MagazineAmmo, ReserveAmmo);
}

bool UGPWeaponComponent::ResolveViewpoint(FVector& OutLocation, FRotator& OutRotation) const
{
    const APawn* PawnOwner = Cast<APawn>(GetOwner());
    if (!PawnOwner)
    {
        return false;
    }

    if (PawnOwner->GetController())
    {
        PawnOwner->GetController()->GetPlayerViewPoint(OutLocation, OutRotation);
    }
    else
    {
        PawnOwner->GetActorEyesViewPoint(OutLocation, OutRotation);
    }
    return true;
}

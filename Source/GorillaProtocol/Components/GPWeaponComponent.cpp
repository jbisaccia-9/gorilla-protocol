#include "Components/GPWeaponComponent.h"

#include "Components/GPHealthComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
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
    Tuning.Damage = FMath::Max(1.0f, Tuning.Damage);
    Tuning.HeadshotMultiplier = FMath::Max(1.0f, Tuning.HeadshotMultiplier);
    Tuning.Range = FMath::Max(100.0f, Tuning.Range);
    Tuning.FireInterval = FMath::Max(0.03f, Tuning.FireInterval);
    Tuning.MagazineSize = FMath::Max(1, Tuning.MagazineSize);
    Tuning.ReloadDuration = FMath::Max(0.1f, Tuning.ReloadDuration);
    StartingReserveAmmo = FMath::Max(0, NewReserveAmmo);
    CurrentBloomDegrees = 0.0f;
    LastBloomUpdateTime = -1.0;
    ShotPatternIndex = 0;
    if (HasBegunPlay())
    {
        MagazineAmmo = Tuning.MagazineSize;
        ReserveAmmo = StartingReserveAmmo;
        OnAmmoChanged.Broadcast(MagazineAmmo, ReserveAmmo);
    }
}

void UGPWeaponComponent::SetAiming(bool bNewAiming)
{
    bAiming = bNewAiming;
}

void UGPWeaponComponent::SetSpreadSeed(int32 NewSeed)
{
    SpreadSeed = NewSeed;
    ShotPatternIndex = 0;
}

float UGPWeaponComponent::GetCurrentSpreadDegrees() const
{
    return CalculateSpreadDegrees(Tuning, bAiming, CurrentBloomDegrees);
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

    RecoverBloom(Now);

    FVector ViewLocation;
    FRotator ViewRotation;
    if (!ResolveViewpoint(ViewLocation, ViewRotation))
    {
        return false;
    }

    LastShotTime = Now;
    --MagazineAmmo;
    OnAmmoChanged.Broadcast(MagazineAmmo, ReserveAmmo);

    const float ShotSpreadDegrees = CalculateSpreadDegrees(Tuning, bAiming, CurrentBloomDegrees);
    const FVector ShotDirection = CalculateShotDirection(ViewRotation.Vector(), ShotSpreadDegrees,
        ShotPatternIndex++, SpreadSeed);
    CurrentBloomDegrees = CalculateBloomAfterShot(Tuning, CurrentBloomDegrees);
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
        if (UPrimitiveComponent* HitComponent = Hit.GetComponent(); HitComponent && HitComponent->IsSimulatingPhysics())
        {
            HitComponent->AddImpulseAtLocation(ShotDirection * Tuning.ImpactImpulse, Hit.ImpactPoint);
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
    ApplyLocalRecoil();
    OnShotResolved.Broadcast(ViewLocation, bHit ? Hit.ImpactPoint : TraceEnd, bHit);
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

void UGPWeaponComponent::RecoverBloom(double Now)
{
    if (LastBloomUpdateTime >= 0.0)
    {
        const float Elapsed = static_cast<float>(FMath::Max(0.0, Now - LastBloomUpdateTime));
        CurrentBloomDegrees = FMath::Max(0.0f,
            CurrentBloomDegrees - Tuning.BloomRecoveryDegreesPerSecond * Elapsed);
    }
    LastBloomUpdateTime = Now;
}

void UGPWeaponComponent::ApplyLocalRecoil()
{
    APawn* PawnOwner = Cast<APawn>(GetOwner());
    APlayerController* PlayerController = PawnOwner ? Cast<APlayerController>(PawnOwner->GetController()) : nullptr;
    if (!PlayerController || !PlayerController->IsLocalController())
    {
        return;
    }

    FRotator RecoiledRotation = PlayerController->GetControlRotation();
    RecoiledRotation.Pitch += Tuning.RecoilPitchDegrees;
    const float YawDirection = (ShotPatternIndex % 2 == 0) ? -1.0f : 1.0f;
    RecoiledRotation.Yaw += Tuning.RecoilYawDegrees * YawDirection;
    PlayerController->SetControlRotation(RecoiledRotation);
}

float UGPWeaponComponent::CalculateSpreadDegrees(const FGPWeaponTuning& WeaponTuning, bool bIsAiming,
    float BloomDegrees)
{
    const float BaseSpread = bIsAiming ? WeaponTuning.AimSpreadDegrees : WeaponTuning.HipSpreadDegrees;
    return FMath::Max(0.0f, BaseSpread) + FMath::Clamp(BloomDegrees, 0.0f,
        FMath::Max(0.0f, WeaponTuning.MaxBloomDegrees));
}

float UGPWeaponComponent::CalculateBloomAfterShot(const FGPWeaponTuning& WeaponTuning, float BloomDegrees)
{
    return FMath::Clamp(BloomDegrees + FMath::Max(0.0f, WeaponTuning.SpreadPerShotDegrees), 0.0f,
        FMath::Max(0.0f, WeaponTuning.MaxBloomDegrees));
}

FVector UGPWeaponComponent::CalculateShotDirection(const FVector& AimDirection, float SpreadDegrees,
    int32 PatternIndex, int32 Seed)
{
    FVector SafeAim = AimDirection.GetSafeNormal();
    if (SafeAim.IsNearlyZero())
    {
        SafeAim = FVector::ForwardVector;
    }

    const uint32 PatternSeed = HashCombineFast(static_cast<uint32>(Seed), static_cast<uint32>(FMath::Max(0, PatternIndex)));
    FRandomStream PatternStream(static_cast<int32>(PatternSeed));
    return PatternStream.VRandCone(SafeAim, FMath::DegreesToRadians(FMath::Max(0.0f, SpreadDegrees))).GetSafeNormal();
}

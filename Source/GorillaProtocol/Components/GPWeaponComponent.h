#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GPWeaponComponent.generated.h"

class UNiagaraSystem;
class USoundBase;

USTRUCT(BlueprintType)
struct FGPWeaponTuning
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1.0"))
    float Damage = 34.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1.0"))
    float HeadshotMultiplier = 2.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="100.0"))
    float Range = 15000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.03"))
    float FireInterval = 0.12f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"))
    float HipSpreadDegrees = 1.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"))
    float AimSpreadDegrees = 0.18f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"))
    float SpreadPerShotDegrees = 0.22f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"))
    float MaxBloomDegrees = 2.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"))
    float BloomRecoveryDegreesPerSecond = 3.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"))
    float RecoilPitchDegrees = 0.42f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"))
    float RecoilYawDegrees = 0.16f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"))
    float ImpactImpulse = 850.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1"))
    int32 MagazineSize = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.1"))
    float ReloadDuration = 1.65f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGPAmmoChanged, int32, MagazineAmmo, int32, ReserveAmmo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGPShotResolved, FVector, TraceStart, FVector, TraceEnd,
    bool, bHitActor);

UCLASS(ClassGroup=(GorillaProtocol), meta=(BlueprintSpawnableComponent))
class GORILLAPROTOCOL_API UGPWeaponComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGPWeaponComponent();

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void StartFire();

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void StopFire();

    UFUNCTION(BlueprintCallable, Category="Weapon")
    bool FireSingleShot();

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void Reload();

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void SetAiming(bool bNewAiming);

    void ConfigureWeapon(const FGPWeaponTuning& NewTuning, int32 NewReserveAmmo);

    UFUNCTION(BlueprintPure, Category="Weapon")
    int32 GetMagazineAmmo() const { return MagazineAmmo; }

    UFUNCTION(BlueprintPure, Category="Weapon")
    int32 GetReserveAmmo() const { return ReserveAmmo; }

    UFUNCTION(BlueprintPure, Category="Weapon")
    bool IsReloading() const { return bReloading; }

    UFUNCTION(BlueprintPure, Category="Weapon")
    float GetFireInterval() const { return Tuning.FireInterval; }

    UFUNCTION(BlueprintPure, Category="Weapon")
    float GetReloadDuration() const { return Tuning.ReloadDuration; }

    UFUNCTION(BlueprintPure, Category="Weapon")
    float GetCurrentSpreadDegrees() const;

    void SetSpreadSeed(int32 NewSeed);

    static float CalculateSpreadDegrees(const FGPWeaponTuning& WeaponTuning, bool bIsAiming,
        float BloomDegrees);
    static float CalculateBloomAfterShot(const FGPWeaponTuning& WeaponTuning, float BloomDegrees);
    static FVector CalculateShotDirection(const FVector& AimDirection, float SpreadDegrees,
        int32 PatternIndex, int32 Seed);

    UPROPERTY(BlueprintAssignable, Category="Weapon")
    FGPAmmoChanged OnAmmoChanged;

    UPROPERTY(BlueprintAssignable, Category="Weapon")
    FGPShotResolved OnShotResolved;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
    FGPWeaponTuning Tuning;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
    int32 StartingReserveAmmo = 120;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
    TObjectPtr<UNiagaraSystem> MuzzleEffect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
    TObjectPtr<UNiagaraSystem> ImpactEffect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Audio")
    TObjectPtr<USoundBase> FireSound;

    UFUNCTION(BlueprintImplementableEvent, Category="Weapon|Presentation")
    void BP_OnWeaponFired(const FHitResult& Hit, bool bHitActor);

private:
    void CompleteReload();
    void HandleAutoFire();
    void RecoverBloom(double Now);
    void ApplyLocalRecoil();
    bool ResolveViewpoint(FVector& OutLocation, FRotator& OutRotation) const;

    FTimerHandle FireTimer;
    FTimerHandle ReloadTimer;
    int32 MagazineAmmo = 0;
    int32 ReserveAmmo = 0;
    double LastShotTime = -100.0;
    double LastBloomUpdateTime = -1.0;
    float CurrentBloomDegrees = 0.0f;
    int32 SpreadSeed = 1337;
    int32 ShotPatternIndex = 0;
    bool bAiming = false;
    bool bReloading = false;
};

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1"))
    int32 MagazineSize = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.1"))
    float ReloadDuration = 1.65f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGPAmmoChanged, int32, MagazineAmmo, int32, ReserveAmmo);

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
    void SetAiming(bool bNewAiming) { bAiming = bNewAiming; }

    void ConfigureWeapon(const FGPWeaponTuning& NewTuning, int32 NewReserveAmmo);

    UFUNCTION(BlueprintPure, Category="Weapon")
    int32 GetMagazineAmmo() const { return MagazineAmmo; }

    UFUNCTION(BlueprintPure, Category="Weapon")
    int32 GetReserveAmmo() const { return ReserveAmmo; }

    UPROPERTY(BlueprintAssignable, Category="Weapon")
    FGPAmmoChanged OnAmmoChanged;

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
    bool ResolveViewpoint(FVector& OutLocation, FRotator& OutRotation) const;

    FTimerHandle FireTimer;
    FTimerHandle ReloadTimer;
    int32 MagazineAmmo = 0;
    int32 ReserveAmmo = 0;
    double LastShotTime = -100.0;
    bool bAiming = false;
    bool bReloading = false;
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GPBrunoCharacter.generated.h"

class UCameraComponent;
class UPointLightComponent;
class USoundBase;
class UStaticMeshComponent;

UCLASS()
class GORILLAPROTOCOL_API AGPBrunoCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AGPBrunoCharacter();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    float GetHealth() const { return Health; }
    float GetMaxHealth() const { return MaxHealth; }
    int32 GetAmmo() const { return Ammo; }
    int32 GetMagazineSize() const { return MagazineSize; }
    const FString& GetSubtitle() const { return CurrentSubtitle; }
    bool IsSubtitleVisible() const;

    void SpeakMissionStart();
    void SpeakObjective();
    void SpeakComplete();

private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void StartSprint();
    void StopSprint();
    void StartFire();
    void ResetFire();
    void Punch();
    void ResetPunch();
    void Interact();
    void Reload();
    void FinishReload();
    void BrunoVoice();
    void Speak(const TCHAR* AssetName, const TCHAR* Subtitle);
    void Die();

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> WeaponMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> LeftForearm;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> RightForearm;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> LeftFist;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> RightFist;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UPointLightComponent> MuzzleLight;

    float MaxHealth = 100.0f;
    float Health = 100.0f;
    float WalkSpeed = 520.0f;
    float SprintSpeed = 820.0f;
    int32 MagazineSize = 12;
    int32 Ammo = 12;
    bool bCanFire = true;
    bool bCanPunch = true;
    bool bSprinting = false;
    bool bReloading = false;
    bool bDead = false;
    float SubtitleUntil = 0.0f;
    FString CurrentSubtitle;
    FTimerHandle FireTimer;
    FTimerHandle ReloadTimer;
    FTimerHandle MuzzleTimer;
    FTimerHandle PunchTimer;
};

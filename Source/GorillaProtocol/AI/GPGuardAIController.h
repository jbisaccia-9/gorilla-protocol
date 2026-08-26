#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AI/GPEncounterSubsystem.h"
#include "Perception/AIPerceptionTypes.h"
#include "GPGuardAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Hearing;
class UAISenseConfig_Sight;
class AGPGuardCharacter;

UENUM(BlueprintType)
enum class EGPGuardState : uint8
{
    Patrol,
    Suspicious,
    Investigate,
    Engage,
    Reposition,
    Telegraph,
    FireBurst,
    Reload,
    Stagger,
    MeleeWindup,
    Dead
};

UCLASS()
class GORILLAPROTOCOL_API AGPGuardAIController : public AAIController
{
    GENERATED_BODY()

public:
    AGPGuardAIController();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintPure, Category="AI")
    EGPGuardState GetGuardState() const { return GuardState; }

    UFUNCTION(BlueprintPure, Category="AI")
    EGPGuardTacticalRole GetTacticalRole() const { return TacticalRole; }

    static float GetDesiredRangeForRole(EGPGuardTacticalRole Role);
    static int32 GetBurstSizeForDistance(float Distance);

    void NotifyDamaged(AActor* DamageCauser);
    void NotifyGuardDeath();

protected:
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
    TObjectPtr<UAIPerceptionComponent> GuardPerceptionComponent;

    UPROPERTY()
    TObjectPtr<UAISenseConfig_Sight> SightConfig;

    UPROPERTY()
    TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="AI")
    EGPGuardState GuardState = EGPGuardState::Patrol;

private:
    UFUNCTION()
    void HandleTargetPerception(AActor* Actor, FAIStimulus Stimulus);

    void SetTarget(AActor* NewTarget);
    void EnterState(EGPGuardState NewState, float Duration = 0.0f);
    void TickPatrol(float Now);
    void TickInvestigate(float Now);
    void TickCombat(float Now);
    void TickReposition(float Now);
    void BeginReposition(float Now, bool bForcedBySuppression);
    void TryAcquireSharedContact();
    void ReportContactToSquad();
    void ReleaseFireToken();
    void ReleaseMeleeToken();
    bool HasValidTarget() const;
    bool CanSeeTarget() const;

    UPROPERTY()
    TObjectPtr<AGPGuardCharacter> GuardPawn;

    UPROPERTY()
    TObjectPtr<AActor> TargetActor;

    FVector PatrolOrigin = FVector::ZeroVector;
    FVector LastKnownLocation = FVector::ZeroVector;
    float StateDeadline = 0.0f;
    float NextDecisionTime = 0.0f;
    float NextAttackTime = 0.0f;
    float LastContactTime = -100.0f;
    float NextRepositionTime = 0.0f;
    int32 BurstRoundsRemaining = 0;
    int32 CompletedBursts = 0;
    uint32 LastObservedSuppressionSerial = 0;
    bool bOwnsFireToken = false;
    bool bOwnsMeleeToken = false;
    bool bHasSightStimulus = false;
    EGPGuardTacticalRole TacticalRole = EGPGuardTacticalRole::Pressure;
    FRandomStream CombatRandom;
};

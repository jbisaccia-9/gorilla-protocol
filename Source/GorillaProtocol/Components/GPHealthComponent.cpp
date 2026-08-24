#include "Components/GPHealthComponent.h"

UGPHealthComponent::UGPHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UGPHealthComponent::BeginPlay()
{
    Super::BeginPlay();
    CurrentHealth = MaxHealth;
}

bool UGPHealthComponent::ReceiveDamage(float Damage, AActor* DamageCauser)
{
    if (Damage <= 0.0f || IsDead() || !GetOwner())
    {
        return false;
    }

    CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.0f, MaxHealth);
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, DamageCauser);
    if (IsDead())
    {
        OnDeath.Broadcast(DamageCauser);
    }
    return true;
}

void UGPHealthComponent::RestoreHealth(float Amount)
{
    if (Amount <= 0.0f || IsDead() || !GetOwner())
    {
        return;
    }

    CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.0f, MaxHealth);
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, nullptr);
}

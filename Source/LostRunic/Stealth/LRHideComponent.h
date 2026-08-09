#pragma once

#include "Components/ActorComponent.h"
#include "Stealth/LRGuardVisibility.h"

#include "LRHideComponent.generated.h"

class ACharacter;
class ALRHidePoint;
class ULRStateComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRHiddenStateChanged, bool, bHidden, ALRHidePoint*, hidePoint);

/** Owns entry, exit, movement locking, and guard visibility while using cover. */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Hide"))
class LOSTRUNIC_API ULRHideComponent : public UActorComponent, public ILRGuardVisibility
{
	GENERATED_BODY()

public:
	ULRHideComponent();

	virtual void BeginPlay() override;
	virtual bool IsVisibleToGuard_Implementation(AActor* guard) const override;

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Stealth")
	bool EnterHidePoint(ALRHidePoint* hidePoint);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Stealth")
	bool ExitHidePoint();

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Stealth")
	bool IsHidden() const { return CurrentHidePoint.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Stealth")
	ALRHidePoint* GetCurrentHidePoint() const { return CurrentHidePoint.Get(); }

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Stealth")
	FLRHiddenStateChanged OnHiddenStateChanged;

private:
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> Character;

	UPROPERTY(Transient)
	TObjectPtr<ULRStateComponent> State;

	UPROPERTY(Transient)
	TWeakObjectPtr<ALRHidePoint> CurrentHidePoint;

	bool bMovementLockedByHide = false;
};

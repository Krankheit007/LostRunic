#pragma once

#include "GameFramework/Character.h"

#include "LRGuardCharacter.generated.h"

class AActor;
class ALRGuardAIController;
class ULRCourageResponseComponent;
class ULRAlertComponent;
class ULRGuardDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRPlayerCaptured, AActor*, playerActor);

/** Thin guard assembly; alert, Courage response, perception, and behavior are delegated to components/controller. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Guard Character"))
class LOSTRUNIC_API ALRGuardCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ALRGuardCharacter();

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI")
	ULRAlertComponent* GetAlertComponent() const { return Alert; }

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|AI")
	bool CaptureTarget(AActor* target);

	AActor* GetPatrolPoint(int32 index) const;
	int32 GetPatrolPointCount() const { return PatrolPoints.Num(); }

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|AI")
	FLRPlayerCaptured OnPlayerCaptured;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	TObjectPtr<ULRGuardDefinition> Definition;

	UPROPERTY(EditInstanceOnly, Category = "Guard|Patrol")
	TArray<TObjectPtr<AActor>> PatrolPoints;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULRAlertComponent> Alert;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULRCourageResponseComponent> CourageResponse;
};

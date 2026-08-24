/**
 * @file LRWorldAlertBarWidgetBase.h
 * @brief World alert presentation bound to the controller's authoritative Awareness snapshot.
 */
#pragma once

#include "AI/LRGuardTypes.h"
#include "Blueprint/UserWidget.h"

#include "LRWorldAlertBarWidgetBase.generated.h"

class ALRGuardAIController;
class ALRGuardCharacter;

UCLASS(Abstract, BlueprintType, meta = (DisplayName = "Lost Runic World Alert Bar Base"))
class LOSTRUNIC_API ULRWorldAlertBarWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|UI|Alert")
	void InitializeForGuard(ALRGuardCharacter* guard);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|UI|Alert")
	void Shutdown();

	UFUNCTION(BlueprintPure, Category = "Lost Runic|UI|Alert")
	const FLRAlertSnapshot& GetCurrentSnapshot() const { return CurrentSnapshot; }

	/** Existing Blueprint presentation contract remains unchanged. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|UI|Alert")
	void HandleAlertSnapshotChanged(const FLRAlertSnapshot& snapshot);

protected:
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleAwarenessChanged(const FLRGuardAwarenessSnapshot& snapshot);

	UPROPERTY(Transient)
	TWeakObjectPtr<ALRGuardAIController> GuardController;

	FLRAlertSnapshot CurrentSnapshot;
};

/**
 * @file LRPerceptionPresentationComponent.h
 * @brief Owns Perception post-process lifetime, player position and fixed eight Echo slots.
 */
#pragma once

#include "Components/ActorComponent.h"
#include "Perception/LRPerceptionTypes.h"

#include "LRPerceptionPresentationComponent.generated.h"

class UCameraComponent;
class UMaterialInstanceDynamic;
class UMaterialParameterCollectionInstance;
class ULRGameContentSet;
class ULRPerceptionEventSubsystem;
class ULRPresentationTuning;
class ULRStateComponent;
class ULRVisualStyleDefinition;

/** Runtime Perception renderer; no permanent Tick and no direct state unlock authority. */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Perception Presentation"))
class LOSTRUNIC_API ULRPerceptionPresentationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	static constexpr int32 MaxEchoSlots = 8;

	ULRPerceptionPresentationComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void TickComponent(float deltaTime, ELevelTick tickType,
		FActorComponentTickFunction* tickFunction) override;

	/** Sets the current player world position; normally called by CharacterMovementUpdated. */
	void SetPlayerPosition(const FVector& newPosition);

	/** Clears all active slots without changing the Perception state machine. */
	void ClearEchoes();

	/** Returns whether the Perception post-process is currently active or transitioning. */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Perception")
	bool IsPerceptionActive() const { return bPerceptionActive; }

	/** Returns the number of slots that have not yet expired. */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Perception")
	int32 GetActiveEchoCount() const;

	/** Selects a material inspection view; it never changes reveal rules. */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Perception|Debug")
	void SetDebugView(ELRPerceptionDebugView newDebugView);

	/** Mirrors the current slots for diagnostics and focused automation tests. */
	const TArray<FLRPerceptionEchoSlot>& GetEchoSlots() const { return EchoSlots; }

private:
	void ResolveRuntimeDependencies();
	void ResolveVisualStyle(const ULRGameContentSet* contentSet);
	void InitializePostProcess();
	void BeginBlend(float targetBlend, float durationSeconds);
	void CompleteBlend();
	void AddBlendable();
	void RemoveBlendable();
	void WriteStyleParameters();
	void WriteRuntimeParameters();
	void ApplyPulse(const FLRPerceptionPulseRequest& request);
	void SpawnPulseVFX(const FLRPerceptionPulseRequest& request);
	void EnterPerception();
	void ExitPerception();
	float GetCurrentGameTime() const;
	int32 FindRefreshSlot(const FLRPerceptionPulseRequest& request, float now) const;
	void SetEchoSlot(int32 slotIndex, const FLRPerceptionPulseRequest& request, float now, bool bRefresh);

	UFUNCTION()
	void HandleStateChanged(ELRPerceptionMode currentMode, FGameplayTag reason);

	UFUNCTION()
	void HandleCharacterMovementUpdated(float deltaSeconds, FVector oldLocation, FVector oldVelocity);

	void HandlePerceptionPulse(const FLRPerceptionPulseRequest& request);

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(Transient)
	TObjectPtr<ULRStateComponent> StateComponent;

	UPROPERTY(Transient)
	TObjectPtr<ULRPresentationTuning> Tuning;

	UPROPERTY(Transient)
	TObjectPtr<ULRVisualStyleDefinition> VisualStyle;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PerceptionPostProcessMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollectionInstance> VisualStyleMPCInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollectionInstance> RuntimeMPCInstance;

	UPROPERTY(Transient)
	TArray<FLRPerceptionEchoSlot> EchoSlots;

	ELRPerceptionDebugView DebugView = ELRPerceptionDebugView::None;
	FVector PlayerPosition = FVector::ZeroVector;
	float CurrentBlend = 0.0f;
	float BlendStart = 0.0f;
	float BlendTarget = 0.0f;
	float BlendDuration = 0.0f;
	float BlendElapsed = 0.0f;
	bool bPerceptionActive = false;
	bool bBlendActive = false;
};

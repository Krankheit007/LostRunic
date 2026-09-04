#pragma once

#include "Camera/LRCutawayTypes.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"

#include "LRCutawayTargetComponent.generated.h"

class UPrimitiveComponent;
class ULRPresentationTuning;

/** Authoring and runtime unit for one cutaway target and its material CPD contract. */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Cutaway Target"))
class LOSTRUNIC_API ULRCutawayTargetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULRCutawayTargetComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* tickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Cutaway")
	bool SetCutawayRequest(UObject* requester, ELRCutawayRequestType requestType, float amount, bool bImmediate = false);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Cutaway")
	void ClearCutawayRequest(UObject* requester, ELRCutawayRequestType requestType, bool bImmediate = false);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Cutaway")
	void SetForegroundCutawayEnabled(bool bEnabled, bool bImmediate = false);

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Cutaway")
	float GetCurrentCutawayAmount(ELRCutawayRequestType requestType) const;

	void SetLocalCenterUV(const FVector2D& centerUV);
	void SetLocalRadiusRefPx(float radiusRefPx);
	bool SupportsRequest(ELRCutawayRequestType requestType) const;

private:
	using FRequestMap = TMap<TWeakObjectPtr<UObject>, float>;

	void ResolvePrimitives();
	void ConfigureDetectionPrimitives();
	void InitializePrimitiveData();
	void ValidateMaterials() const;
	void AuditRequesterLifetimes();
	void RefreshRequesterAuditTimer();
	bool HasExternalRequests() const;
	void UpdateChannelTarget(ELRCutawayRequestType requestType, bool bImmediate);
	void WriteAmount(ELRCutawayRequestType requestType, float amount);
	void WritePrimitiveDataFloat(int32 index, float value);
	FRequestMap& GetRequestMap(ELRCutawayRequestType requestType);
	const FLRCutawayChannelState& GetChannel(ELRCutawayRequestType requestType) const;
	FLRCutawayChannelState& GetMutableChannel(ELRCutawayRequestType requestType);
	float GetRequestedAmount(FRequestMap& requests);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cutaway|Modes", meta = (AllowPrivateAccess = "true"))
	bool bEnableLocalCutaway = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cutaway|Modes", meta = (AllowPrivateAccess = "true"))
	bool bEnableGroupCutaway = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cutaway|Modes", meta = (AllowPrivateAccess = "true"))
	bool bEnableForegroundCutaway = false;

	UPROPERTY(EditAnywhere, Category = "Cutaway|Primitives")
	TArray<FComponentReference> AffectedPrimitiveReferences;

	UPROPERTY(EditAnywhere, Category = "Cutaway|Primitives")
	TArray<FComponentReference> DetectionPrimitiveReferences;

	/** Slots listed here remain visible even when the rest of the primitive cuts away. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cutaway|Materials", meta = (AllowPrivateAccess = "true"))
	TArray<FName> PersistentMaterialSlots;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cutaway|Root Preserve", meta = (AllowPrivateAccess = "true"))
	bool bOverrideRootPreserve = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cutaway|Root Preserve", meta = (AllowPrivateAccess = "true", EditCondition = "bOverrideRootPreserve", ClampMin = "0.0", ClampMax = "500.0", Units = "cm"))
	float RootHeightCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cutaway|Root Preserve", meta = (AllowPrivateAccess = "true", EditCondition = "bOverrideRootPreserve", ClampMin = "0.0", ClampMax = "100.0", Units = "cm"))
	float RootFeatherCm = 0.0f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPrimitiveComponent>> AffectedPrimitives;

	UPROPERTY(Transient)
	TObjectPtr<ULRPresentationTuning> Tuning;

	FRequestMap LocalRequests;
	FRequestMap GroupRequests;
	FRequestMap ForegroundRequests;
	FLRCutawayChannelState LocalState;
	FLRCutawayChannelState GroupState;
	FLRCutawayChannelState ForegroundState;
	float HideDuration = 0.25f;
	float RestoreDuration = 0.35f;
	FTimerHandle RequesterAuditTimer;
};

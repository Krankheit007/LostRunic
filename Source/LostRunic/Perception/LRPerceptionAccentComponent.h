/**
 * @file LRPerceptionAccentComponent.h
 * @brief Owns Stencil 3 Narrative Accent custom-depth state for one actor during Perception.
 */
#pragma once

#include "Components/ActorComponent.h"

#include "LRPerceptionAccentComponent.generated.h"

class UPrimitiveComponent;

/** Saved writer state for one primitive while Narrative Accent owns CustomDepth. */
struct FLRPerceptionAccentPrimitiveState
{
	TWeakObjectPtr<UPrimitiveComponent> Primitive;
	bool bRenderCustomDepth = false;
	int32 StencilValue = 0;
};

/** Explicit Stencil 3 owner; it never changes the stencil scheme into a bitfield. */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Perception Narrative Accent"))
class LOSTRUNIC_API ULRPerceptionAccentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULRPerceptionAccentComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	/** Enables Stencil 3 and CustomDepth for this actor during Perception. */
	void ActivateNarrativeAccent();

	/** Restores the exact pre-accent CustomDepth/Stencil state. */
	void DeactivateNarrativeAccent();

	bool IsNarrativeAccentActive() const { return bAccentActive; }

	/** When true all owner primitives are accented; otherwise only PerceptionAccent-tagged primitives are used. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception|Accent")
	bool bUseAllOwnerPrimitives = true;

private:
	void CachePrimitives();

	TArray<FLRPerceptionAccentPrimitiveState> PrimitiveStates;

	bool bAccentActive = false;
};

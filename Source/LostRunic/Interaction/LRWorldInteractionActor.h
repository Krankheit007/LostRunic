#pragma once

#include "GameFramework/Actor.h"
#include "Interaction/LRInteractable.h"

#include "LRWorldInteractionActor.generated.h"

class UStaticMeshComponent;
class USphereComponent;

/** 关卡装配可选择的稳定领域动作；实际规则仍由对应组件或 Subsystem 执行。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic World Interaction Action"))
enum class ELRWorldInteractionAction : uint8
{
	Dialogue UMETA(DisplayName = "Start Dialogue"),
	Reading UMETA(DisplayName = "Start Reading"),
	PickupItem UMETA(DisplayName = "Pickup Item"),
	PickupNote UMETA(DisplayName = "Pickup Note"),
	PickupCollectible UMETA(DisplayName = "Pickup Collectible"),
	CompleteLevelEvent UMETA(DisplayName = "Complete Level Event"),
	SetResumeAnchor UMETA(DisplayName = "Set Resume Anchor"),
	CommitMemoryEvent UMETA(DisplayName = "Commit Memory Event"),
	ReturnFromMemory UMETA(DisplayName = "Return From Memory")
};

/**
 * 把关卡中的一次交互路由到已有领域系统。关卡实例只配置稳定 ID、提示和外观，
 * 不在 Level Blueprint 中复制库存、叙事或存档规则。
 */
UCLASS(Blueprintable, meta = (DisplayName = "Lost Runic World Interaction"))
class LOSTRUNIC_API ALRWorldInteractionActor : public AActor, public ILRInteractable
{
	GENERATED_BODY()

public:
	ALRWorldInteractionActor();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	virtual TArray<FLRInteractionOption> GetInteractionOptions_Implementation(AActor* interactor) override;
	virtual FVector GetInteractionLocation_Implementation() override;
	virtual FLRInteractionResult ExecuteInteraction_Implementation(AActor* interactor,
		FGameplayTag actionTag) override;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Interaction")
	bool IsCompleted() const { return bCompleted; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	/** 可选的无 Tick 自动触发范围；仅在 bAutoExecuteOnPlayerOverlap 为 true 时启用。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> AutoTrigger;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FLRInteractionOption InteractionOption;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	ELRWorldInteractionAction Action = ELRWorldInteractionAction::Dialogue;

	/** 对话、阅读、物品、笔记、收藏品、事件或锚点的稳定 ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FName ContentId = NAME_None;

	/** 对话/阅读完成时提交的可选关卡事件。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FName CompletionEventId = NAME_None;

	/** 检查点使用的地图 ID；其他动作忽略。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Save")
	FName MapId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bOneShot = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Presentation")
	bool bHideAfterSuccess = true;

	/** Pawn 首次进入范围时执行与手动交互相同的领域动作。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Automatic")
	bool bAutoExecuteOnPlayerOverlap = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Automatic",
		meta = (ClampMin = "10.0", ClampMax = "1000.0", Units = "cm"))
	float AutoTriggerRadius = 120.0f;

private:
	UFUNCTION()
	void HandleAutoTriggerBeginOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor,
		UPrimitiveComponent* otherComponent, int32 otherBodyIndex, bool bFromSweep, const FHitResult& sweepResult);

	bool ExecuteDomainAction(AActor* interactor, FGameplayTag& outFailureReason);
	void CompletePresentation();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Interaction",
		meta = (AllowPrivateAccess = "true"))
	bool bCompleted = false;
};

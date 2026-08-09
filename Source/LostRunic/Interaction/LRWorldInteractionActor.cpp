#include "Interaction/LRWorldInteractionActor.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Framework/LRCharacter.h"
#include "Items/LRInventoryComponent.h"
#include "Narrative/LRDialogueSubsystem.h"
#include "Save/LRSaveSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"

ALRWorldInteractionActor::ALRWorldInteractionActor()
{
	PrimaryActorTick.bCanEverTick = false;
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	SetRootComponent(VisualMesh);
	VisualMesh->SetCollisionProfileName(TEXT("BlockAll"));
	AutoTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("AutoTrigger"));
	AutoTrigger->SetupAttachment(VisualMesh);
	AutoTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionOption.ActionTag = LRGameplayTags::InteractionActionUse;
}

void ALRWorldInteractionActor::BeginPlay()
{
	Super::BeginPlay();
	if (!bAutoExecuteOnPlayerOverlap)
	{
		return;
	}
	AutoTrigger->SetSphereRadius(AutoTriggerRadius);
	AutoTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AutoTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	AutoTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	AutoTrigger->OnComponentBeginOverlap.AddDynamic(this, &ALRWorldInteractionActor::HandleAutoTriggerBeginOverlap);
}

void ALRWorldInteractionActor::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (AutoTrigger)
	{
		AutoTrigger->OnComponentBeginOverlap.RemoveAll(this);
	}
	Super::EndPlay(endPlayReason);
}

TArray<FLRInteractionOption> ALRWorldInteractionActor::GetInteractionOptions_Implementation(AActor* interactor)
{
	return bAutoExecuteOnPlayerOverlap || (bCompleted && bOneShot) ? TArray<FLRInteractionOption>()
		: TArray<FLRInteractionOption>({ InteractionOption });
}

void ALRWorldInteractionActor::HandleAutoTriggerBeginOverlap(UPrimitiveComponent* overlappedComponent,
	AActor* otherActor, UPrimitiveComponent* otherComponent, const int32 otherBodyIndex,
	const bool bFromSweep, const FHitResult& sweepResult)
{
	if (!Cast<ALRCharacter>(otherActor) || (bCompleted && bOneShot))
	{
		return;
	}
	ExecuteInteraction_Implementation(otherActor, InteractionOption.ActionTag);
}

FVector ALRWorldInteractionActor::GetInteractionLocation_Implementation()
{
	return GetActorLocation();
}

FLRInteractionResult ALRWorldInteractionActor::ExecuteInteraction_Implementation(AActor* interactor,
	const FGameplayTag actionTag)
{
	FLRInteractionResult result;
	result.ActionTag = actionTag;
	result.EventId = CompletionEventId.IsNone() ? ContentId : CompletionEventId;
	if (bCompleted && bOneShot)
	{
		result.FailureReason = LRGameplayTags::InteractionRejectCompleted;
		return result;
	}

	result.bSuccess = ExecuteDomainAction(interactor, result.FailureReason);
	if (result.bSuccess && bOneShot)
	{
		bCompleted = true;
		CompletePresentation();
	}
	UE_CLOG(!result.bSuccess, LogLostRunicInteraction, Warning,
		TEXT("World interaction=%s action=%d content=%s rejected reason=%s"), *GetNameSafe(this),
		static_cast<int32>(Action), *ContentId.ToString(), *result.FailureReason.ToString());
	return result;
}

bool ALRWorldInteractionActor::ExecuteDomainAction(AActor* interactor, FGameplayTag& outFailureReason)
{
	ALRCharacter* character = Cast<ALRCharacter>(interactor);
	UGameInstance* gameInstance = GetGameInstance();
	ULRDialogueSubsystem* dialogue = gameInstance ? gameInstance->GetSubsystem<ULRDialogueSubsystem>() : nullptr;
	ULRSaveSubsystem* save = gameInstance ? gameInstance->GetSubsystem<ULRSaveSubsystem>() : nullptr;
	ULRInventoryComponent* inventory = character ? character->GetInventoryComponent() : nullptr;

	if (Action == ELRWorldInteractionAction::Dialogue || Action == ELRWorldInteractionAction::Reading)
	{
		const FLRNarrativeResult narrativeResult = !dialogue ? FLRNarrativeResult()
			: Action == ELRWorldInteractionAction::Dialogue
				? dialogue->StartDialogue(ContentId, CompletionEventId)
				: dialogue->StartReading(ContentId, CompletionEventId);
		if (narrativeResult.bSuccess && Action == ELRWorldInteractionAction::Reading && inventory)
		{
			inventory->AddNoteId(ContentId);
		}
		outFailureReason = narrativeResult.FailureReason;
		return narrativeResult.bSuccess;
	}
	if (Action == ELRWorldInteractionAction::PickupItem)
	{
		const bool bAdded = inventory && inventory->AddItem(ContentId);
		if (bAdded)
		{
			inventory->AssignQuickSlot(0, ContentId);
		}
		outFailureReason = bAdded ? FGameplayTag() : LRGameplayTags::InteractionRejectItem;
		return bAdded;
	}
	if (Action == ELRWorldInteractionAction::PickupNote)
	{
		const bool bAdded = inventory && inventory->AddNoteId(ContentId);
		outFailureReason = bAdded ? FGameplayTag() : LRGameplayTags::InteractionRejectCompleted;
		return bAdded;
	}
	if (Action == ELRWorldInteractionAction::PickupCollectible)
	{
		const bool bAdded = inventory && inventory->AddCollectibleId(ContentId);
		if (bAdded && dialogue && !CompletionEventId.IsNone())
		{
			dialogue->TryCompleteEvent(CompletionEventId);
		}
		outFailureReason = bAdded ? FGameplayTag() : LRGameplayTags::InteractionRejectCompleted;
		return bAdded;
	}
	if (Action == ELRWorldInteractionAction::CompleteLevelEvent)
	{
		const FLRNarrativeResult eventResult = dialogue ? dialogue->TryCompleteEvent(ContentId) : FLRNarrativeResult();
		outFailureReason = eventResult.FailureReason;
		return eventResult.bSuccess;
	}
	if (Action == ELRWorldInteractionAction::SetResumeAnchor)
	{
		if (!save || MapId.IsNone() || ContentId.IsNone())
		{
			outFailureReason = LRGameplayTags::InteractionRejectState;
			return false;
		}
		FLRResumeAnchor anchor;
		anchor.MapId = MapId;
		anchor.AnchorId = ContentId;
		anchor.Location = character ? character->GetActorLocation() : GetActorLocation();
		anchor.Rotation = character ? character->GetActorRotation() : GetActorRotation();
		save->SetResumeAnchor(anchor);
		return save->RequestAutoSave(ContentId) == ELRSaveRequestResult::Scheduled;
	}
	if (Action == ELRWorldInteractionAction::CommitMemoryEvent)
	{
		const bool bCommitted = save && save->CommitMemoryEvent(ContentId);
		outFailureReason = bCommitted ? FGameplayTag() : LRGameplayTags::InteractionRejectState;
		return bCommitted;
	}
	if (Action == ELRWorldInteractionAction::ReturnFromMemory)
	{
		const bool bReturning = save && save->RequestReturnFromMemory();
		outFailureReason = bReturning ? FGameplayTag() : LRGameplayTags::InteractionRejectState;
		return bReturning;
	}
	outFailureReason = LRGameplayTags::InteractionRejectState;
	return false;
}

void ALRWorldInteractionActor::CompletePresentation()
{
	if (bHideAfterSuccess)
	{
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
	}
}

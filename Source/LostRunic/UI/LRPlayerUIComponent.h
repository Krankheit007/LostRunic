#pragma once

#include "Components/ActorComponent.h"
#include "Items/LRItemUseTypes.h"
#include "Narrative/LRNarrativeTypes.h"
#include "UI/LRUITypes.h"

#include "LRPlayerUIComponent.generated.h"

class ALRCharacter;
class ALRPlayerController;

/** Coordinates UI domain actions while the owning PlayerController retains input mode and focus ownership. */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Player UI"))
class LOSTRUNIC_API ULRPlayerUIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULRPlayerUIComponent();

	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	void InitializeUI(ALRPlayerController* playerController);
	void SetObservedCharacter(ALRCharacter* character);
	void HandleConfirm();
	void HandleCancel();
	void HandleOpenJournal();
	void HandlePause();
	void OpenMenuScreen(ELRScreenType screen);
	void CloseMenuScreen();
	FLRItemUseResult UseInventoryItem(FName itemId) const;

private:
	UFUNCTION()
	void HandleNarrativePageChanged(FLRNarrativePage page);

	UFUNCTION()
	void HandleNarrativeSessionEnded(ELRNarrativeSessionType sessionType, FName finalContentId);

	class ALRHUD* GetLRHUD() const;
	void UnbindNarrative();

	TWeakObjectPtr<ALRPlayerController> OwnerController;
	TWeakObjectPtr<class ULRDialogueSubsystem> DialogueSubsystem;
};

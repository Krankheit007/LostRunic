#pragma once

#include "Narrative/LRNarrativeTypes.h"
#include "UI/LRUITypes.h"
#include "UObject/Object.h"

#include "LRDialogueWidgetController.generated.h"

class ULRDialogueSubsystem;
class ULRUITuning;
class UWorld;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRNarrativePresentationChanged, FLRNarrativePresentation, presentation);

/** Owns timer-driven typewriter presentation and delegates page advancement to narrative rules. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Dialogue Widget Controller"))
class LOSTRUNIC_API ULRDialogueWidgetController : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(ULRDialogueSubsystem* dialogueSubsystem, ULRUITuning* tuning, UWorld* world);
	void Deinitialize();

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Narrative")
	FLRNarrativeResult HandleConfirm();

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Narrative")
	FLRNarrativeResult SelectChoice(FName choiceId);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Narrative")
	void EndSession();

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Narrative")
	FLRNarrativePresentation GetPresentation() const { return Presentation; }

	void RevealCurrentText();
	void AdvanceTypewriterForTest(float elapsedSeconds);

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Narrative")
	FLRNarrativePresentationChanged OnPresentationChanged;

private:
	UFUNCTION()
	void HandlePageChanged(FLRNarrativePage page);

	UFUNCTION()
	void HandleSessionEnded(ELRNarrativeSessionType sessionType, FName finalContentId);

	void RefreshTypewriter();
	void UpdateVisibleCharacters(int32 visibleCharacterCount);
	void StopTypewriter();
	float GetCharactersPerSecond() const;
	float GetUpdateSeconds() const;

	UPROPERTY(Transient)
	TObjectPtr<ULRDialogueSubsystem> DialogueSubsystem;

	UPROPERTY(Transient)
	TObjectPtr<ULRUITuning> Tuning;

	TWeakObjectPtr<UWorld> World;
	FLRNarrativePresentation Presentation;
	FString FullText;
	double PageStartedAtSeconds = 0.0;
	FTimerHandle TypewriterTimer;
};

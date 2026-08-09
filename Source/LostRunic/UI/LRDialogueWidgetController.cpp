#include "UI/LRDialogueWidgetController.h"

#include "Data/LRUITuning.h"
#include "Engine/World.h"
#include "Narrative/LRDialogueSubsystem.h"
#include "TimerManager.h"

void ULRDialogueWidgetController::Initialize(ULRDialogueSubsystem* dialogueSubsystem, ULRUITuning* tuning, UWorld* world)
{
	Deinitialize();
	DialogueSubsystem = dialogueSubsystem;
	Tuning = tuning;
	World = world;
	if (DialogueSubsystem)
	{
		DialogueSubsystem->OnPageChanged.AddDynamic(this, &ULRDialogueWidgetController::HandlePageChanged);
		DialogueSubsystem->OnSessionEnded.AddDynamic(this, &ULRDialogueWidgetController::HandleSessionEnded);
	}
}

void ULRDialogueWidgetController::Deinitialize()
{
	StopTypewriter();
	if (DialogueSubsystem)
	{
		DialogueSubsystem->OnPageChanged.RemoveDynamic(this, &ULRDialogueWidgetController::HandlePageChanged);
		DialogueSubsystem->OnSessionEnded.RemoveDynamic(this, &ULRDialogueWidgetController::HandleSessionEnded);
	}
	DialogueSubsystem = nullptr;
	Tuning = nullptr;
	World.Reset();
	Presentation = FLRNarrativePresentation();
	FullText.Reset();
}

FLRNarrativeResult ULRDialogueWidgetController::HandleConfirm()
{
	if (!DialogueSubsystem || !DialogueSubsystem->HasActiveSession())
	{
		return FLRNarrativeResult();
	}
	if (!Presentation.bTextFullyRevealed)
	{
		RevealCurrentText();
		FLRNarrativeResult result;
		result.bSuccess = true;
		result.Action = ELRNarrativeAction::RevealCurrentText;
		result.ContentId = Presentation.Page.ContentId;
		return result;
	}
	return DialogueSubsystem->Advance();
}

FLRNarrativeResult ULRDialogueWidgetController::SelectChoice(const FName choiceId)
{
	return DialogueSubsystem ? DialogueSubsystem->SelectChoice(choiceId) : FLRNarrativeResult();
}

void ULRDialogueWidgetController::EndSession()
{
	if (DialogueSubsystem)
	{
		DialogueSubsystem->EndSession();
	}
}

void ULRDialogueWidgetController::RevealCurrentText()
{
	UpdateVisibleCharacters(FullText.Len());
	StopTypewriter();
}

void ULRDialogueWidgetController::AdvanceTypewriterForTest(const float elapsedSeconds)
{
	UpdateVisibleCharacters(FMath::FloorToInt(FMath::Max(0.0f, elapsedSeconds) * GetCharactersPerSecond()));
}

void ULRDialogueWidgetController::HandlePageChanged(const FLRNarrativePage page)
{
	StopTypewriter();
	Presentation = FLRNarrativePresentation();
	Presentation.Page = page;
	FullText = page.Text.ToString();
	PageStartedAtSeconds = World.IsValid() ? World->GetRealTimeSeconds() : 0.0;
	UpdateVisibleCharacters(0);
	if (FullText.IsEmpty())
	{
		return;
	}
	if (World.IsValid())
	{
		World->GetTimerManager().SetTimer(TypewriterTimer, this, &ULRDialogueWidgetController::RefreshTypewriter,
			GetUpdateSeconds(), true);
	}
}

void ULRDialogueWidgetController::HandleSessionEnded(const ELRNarrativeSessionType sessionType, const FName finalContentId)
{
	StopTypewriter();
	Presentation = FLRNarrativePresentation();
	FullText.Reset();
	OnPresentationChanged.Broadcast(Presentation);
}

void ULRDialogueWidgetController::RefreshTypewriter()
{
	if (!World.IsValid())
	{
		StopTypewriter();
		return;
	}
	const double elapsedSeconds = World->GetRealTimeSeconds() - PageStartedAtSeconds;
	UpdateVisibleCharacters(FMath::FloorToInt(static_cast<float>(elapsedSeconds) * GetCharactersPerSecond()));
}

void ULRDialogueWidgetController::UpdateVisibleCharacters(const int32 visibleCharacterCount)
{
	const int32 clampedCount = FMath::Clamp(visibleCharacterCount, 0, FullText.Len());
	const bool bWasFullyRevealed = Presentation.bTextFullyRevealed;
	Presentation.DisplayedText = FText::FromString(FullText.Left(clampedCount));
	Presentation.bTextFullyRevealed = clampedCount == FullText.Len();
	OnPresentationChanged.Broadcast(Presentation);
	if (Presentation.bTextFullyRevealed && !bWasFullyRevealed)
	{
		StopTypewriter();
	}
}

void ULRDialogueWidgetController::StopTypewriter()
{
	if (World.IsValid())
	{
		World->GetTimerManager().ClearTimer(TypewriterTimer);
	}
}

float ULRDialogueWidgetController::GetCharactersPerSecond() const
{
	return Tuning ? Tuning->TypewriterCharactersPerSecond : 30.0f;
}

float ULRDialogueWidgetController::GetUpdateSeconds() const
{
	return Tuning ? Tuning->TypewriterUpdateSeconds : 0.033f;
}

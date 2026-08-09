#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "LRInputConfig.generated.h"

class UInputAction;
class UInputMappingContext;

/** Semantic input assets shared by keyboard, mouse, and gamepad mappings. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Input Config"))
class LOSTRUNIC_API ULRInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Contexts")
	TObjectPtr<UInputMappingContext> GameplayContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Contexts")
	TObjectPtr<UInputMappingContext> DialogueContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Contexts")
	TObjectPtr<UInputMappingContext> MenuContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Contexts")
	TObjectPtr<UInputMappingContext> TransitionContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Movement")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Movement")
	TObjectPtr<UInputAction> SneakAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Movement")
	TObjectPtr<UInputAction> RunAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Gameplay")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|State")
	TObjectPtr<UInputAction> CloseEyesAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|State")
	TObjectPtr<UInputAction> OpenEyesAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|UI")
	TObjectPtr<UInputAction> ConfirmAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|UI")
	TObjectPtr<UInputAction> CancelAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Gameplay")
	TArray<TObjectPtr<UInputAction>> QuickSlotActions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Gameplay")
	TObjectPtr<UInputAction> UseQuickSlotAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Gameplay")
	TObjectPtr<UInputAction> PreviousQuickSlotAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Gameplay")
	TObjectPtr<UInputAction> NextQuickSlotAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Gameplay")
	TObjectPtr<UInputAction> ToggleCrouchAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|UI")
	TObjectPtr<UInputAction> OpenJournalAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|UI")
	TObjectPtr<UInputAction> PauseAction;

	bool Validate(FString& outError) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
};

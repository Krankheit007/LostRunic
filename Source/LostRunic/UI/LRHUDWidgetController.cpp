/**
 * @file LRHUDWidgetController.cpp
 * @brief 实现 HUD、状态遮罩、对话/阅读、背包/笔记/收藏、暂停、存档槽和过场的控制器边界。UI 订阅领域事件并负责表现，不参与核心规则判定。
 *
 * 关联文件：LRHUDWidgetController.h；所属领域：UI。
 * 设计依据：Docs/Technical/08_ArchitectureBoundaries.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "UI/LRHUDWidgetController.h"

#include "EnhancedInputSubsystems.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRPresentationTuning.h"
#include "Engine/Engine.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Framework/LRCharacter.h"
#include "Framework/LRPlayerController.h"
#include "Interaction/LRInteractionComponent.h"
#include "Input/LRInputConfig.h"
#include "InputCoreTypes.h"
#include "InputAction.h"
#include "State/LRStateComponent.h"
#include "UserSettings/EnhancedInputUserSettings.h"

/**
 * @brief 更新 Observed Character，并在需要时同步组件状态或广播变化事件。
 * @param character 参与本次操作的运行时对象 `character`；函数会检查空值和所需接口。
 */
void ULRHUDWidgetController::SetObservedCharacter(ALRCharacter* character)
{
	Deinitialize();
	ObservedCharacter = character;
	ULRStateComponent* state = character ? character->GetStateComponent() : nullptr;
	if (!state)
	{
		return;
	}
	CurrentMode = state->GetCurrentMode();
	ObservedStateComponent = state;
	if (character->GetWorld())
	{
		const UGameInstance* gameInstance = character->GetWorld()->GetGameInstance();
		const ULRGameInstanceSubsystem* dataSubsystem = gameInstance
			? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
		PresentationTuning = dataSubsystem && dataSubsystem->GetTuningSet()
			? dataSubsystem->GetTuningSet()->Presentation : nullptr;
	}
	state->OnStateChanged.AddDynamic(this, &ULRHUDWidgetController::HandleStateChanged);
	state->OnHoldStarted.AddDynamic(this, &ULRHUDWidgetController::HandleHoldStarted);
	state->OnHoldCanceled.AddDynamic(this, &ULRHUDWidgetController::HandleHoldCanceled);
	state->OnHoldThresholdReached.AddDynamic(this, &ULRHUDWidgetController::HandleHoldThresholdReached);
	state->OnStateChangeRejected.AddDynamic(this, &ULRHUDWidgetController::HandleStateChangeRejected);
	ObservedPlayerController = character ? Cast<ALRPlayerController>(character->GetController()) : nullptr;
	if (ALRPlayerController* playerController = ObservedPlayerController.Get())
	{
		CurrentInputMode = playerController->GetLRInputMode();
		playerController->OnInputModeChanged.AddDynamic(this, &ULRHUDWidgetController::HandleInputModeChanged);

		if (ULocalPlayer* localPlayer = playerController->GetLocalPlayer())
		{
			EnhancedInputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(localPlayer);
		}
		if (GEngine)
		{
			InputDeviceSubsystem = GEngine->GetEngineSubsystem<UInputDeviceSubsystem>();
		}
		if (UInputDeviceSubsystem* inputDeviceSubsystem = InputDeviceSubsystem.Get())
		{
			inputDeviceSubsystem->OnInputHardwareDeviceChanged.AddDynamic(
				this, &ULRHUDWidgetController::HandleInputHardwareDeviceChanged);
		}
		if (UEnhancedInputLocalPlayerSubsystem* enhancedInput = EnhancedInputSubsystem.Get())
		{
			enhancedInput->ControlMappingsRebuiltDelegate.AddDynamic(
				this, &ULRHUDWidgetController::HandleControlMappingsRebuilt);
			enhancedInput->OnPostUserSettingsInitialized.AddDynamic(
				this, &ULRHUDWidgetController::HandleUserSettingsInitialized);
			HandleUserSettingsInitialized(enhancedInput->GetUserSettings());
		}
	}
	if (ULRInteractionComponent* interaction = character->GetInteractionComponent())
	{
		interaction->OnFocusedInteractionChanged.AddDynamic(this, &ULRHUDWidgetController::HandleFocusedInteractionChanged);
		SourceInteractionFocus = interaction->GetFocusSnapshot();
	}
	RefreshInteractionDisplayKey();
}

/**
 * @brief 释放子系统事件绑定和运行时缓存。
 */
void ULRHUDWidgetController::Deinitialize()
{
	if (ULRStateComponent* state = ObservedStateComponent.Get())
	{
		state->OnStateChanged.RemoveDynamic(this, &ULRHUDWidgetController::HandleStateChanged);
		state->OnHoldStarted.RemoveDynamic(this, &ULRHUDWidgetController::HandleHoldStarted);
		state->OnHoldCanceled.RemoveDynamic(this, &ULRHUDWidgetController::HandleHoldCanceled);
		state->OnHoldThresholdReached.RemoveDynamic(this, &ULRHUDWidgetController::HandleHoldThresholdReached);
		state->OnStateChangeRejected.RemoveDynamic(this, &ULRHUDWidgetController::HandleStateChangeRejected);
	}
	if (ALRCharacter* character = ObservedCharacter.Get())
	{
		if (ULRInteractionComponent* interaction = character->GetInteractionComponent())
		{
			interaction->OnFocusedInteractionChanged.RemoveDynamic(this, &ULRHUDWidgetController::HandleFocusedInteractionChanged);
		}
	}
	if (ALRPlayerController* playerController = ObservedPlayerController.Get())
	{
		playerController->OnInputModeChanged.RemoveDynamic(this, &ULRHUDWidgetController::HandleInputModeChanged);
	}
	if (UInputDeviceSubsystem* inputDeviceSubsystem = InputDeviceSubsystem.Get())
	{
		inputDeviceSubsystem->OnInputHardwareDeviceChanged.RemoveDynamic(
			this, &ULRHUDWidgetController::HandleInputHardwareDeviceChanged);
	}
	if (UEnhancedInputLocalPlayerSubsystem* enhancedInput = EnhancedInputSubsystem.Get())
	{
		enhancedInput->ControlMappingsRebuiltDelegate.RemoveDynamic(
			this, &ULRHUDWidgetController::HandleControlMappingsRebuilt);
		enhancedInput->OnPostUserSettingsInitialized.RemoveDynamic(
			this, &ULRHUDWidgetController::HandleUserSettingsInitialized);
	}
	if (UEnhancedInputUserSettings* settings = EnhancedInputUserSettings.Get())
	{
		settings->OnSettingsChanged.RemoveDynamic(this, &ULRHUDWidgetController::HandleUserSettingsChanged);
	}
	ObservedCharacter.Reset();
	ObservedStateComponent.Reset();
	ObservedPlayerController.Reset();
	EnhancedInputSubsystem.Reset();
	EnhancedInputUserSettings.Reset();
	InputDeviceSubsystem.Reset();
	PresentationTuning.Reset();
	CurrentMode = ELRPerceptionMode::Normal;
	CurrentInputMode = ELRInputMode::Gameplay;
	SourceInteractionFocus = FLRInteractionFocusSnapshot();
	CurrentInteractionPrompt = FLRInteractionPromptView();
}

/**
 * @brief 处理 Handle State Changed 事件，将引擎回调转换为对应领域状态更新。
 * @param currentMode 本次操作使用的 `currentMode` 枚举或模式值。
 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
 */
void ULRHUDWidgetController::HandleStateChanged(const ELRPerceptionMode currentMode, const FGameplayTag reason)
{
	CurrentMode = currentMode;
	OnPerceptionModeChanged.Broadcast(CurrentMode, reason);
}

float ULRHUDWidgetController::GetEyeOverlaySuccessTailSeconds() const
{
	const ULRPresentationTuning* tuning = PresentationTuning.Get();
	return tuning ? FMath::Max(0.0f, tuning->EyeOverlaySuccessTailSeconds) : 0.12f;
}

float ULRHUDWidgetController::GetEyeOverlayCancelSeconds() const
{
	const ULRPresentationTuning* tuning = PresentationTuning.Get();
	return tuning ? FMath::Max(0.0f, tuning->EyeOverlayCancelSeconds) : 0.15f;
}

float ULRHUDWidgetController::GetEyeOpenThresholdVisualProgress() const
{
	const ULRPresentationTuning* tuning = PresentationTuning.Get();
	return tuning ? FMath::Clamp(tuning->EyeOpenThresholdVisualProgress, 0.0f, 1.0f) : 0.90f;
}

FLinearColor ULRHUDWidgetController::GetEyeOverlayTint(const ELRPerceptionMode mode) const
{
	if (!PresentationTuning.IsValid())
	{
		return FLinearColor::Black;
	}

	switch (mode)
	{
	case ELRPerceptionMode::Perception:
		return PresentationTuning->PerceptionEyeOverlayStyle.Tint;
	case ELRPerceptionMode::Courage:
		return PresentationTuning->CourageEyeOverlayStyle.Tint;
	case ELRPerceptionMode::Memory:
		return PresentationTuning->MemoryEyeOverlayStyle.Tint;
	case ELRPerceptionMode::Normal:
	default:
		return FLinearColor::Black;
	}
}

float ULRHUDWidgetController::GetEyeOverlayMaxOpacity(const ELRPerceptionMode mode) const
{
	if (!PresentationTuning.IsValid() || mode == ELRPerceptionMode::Normal)
	{
		return 0.80f;
	}

	const FLRStateEyeOverlayStyle* style = nullptr;
	switch (mode)
	{
	case ELRPerceptionMode::Perception:
		style = &PresentationTuning->PerceptionEyeOverlayStyle;
		break;
	case ELRPerceptionMode::Courage:
		style = &PresentationTuning->CourageEyeOverlayStyle;
		break;
	case ELRPerceptionMode::Memory:
		style = &PresentationTuning->MemoryEyeOverlayStyle;
		break;
	case ELRPerceptionMode::Normal:
	default:
		break;
	}
	return style ? FMath::Clamp(style->MaxOpacity, 0.0f, 1.0f) : 0.80f;
}

void ULRHUDWidgetController::HandleHoldStarted(const ELRStateRequestType inputType,
	const ELRPerceptionMode targetMode, const float holdSeconds)
{
	OnHoldStarted.Broadcast(inputType, targetMode, holdSeconds);
}

void ULRHUDWidgetController::HandleHoldCanceled(const ELRStateRequestType inputType)
{
	OnHoldCanceled.Broadcast(inputType);
}

void ULRHUDWidgetController::HandleHoldThresholdReached(const ELRStateRequestType inputType,
	const ELRPerceptionMode targetMode)
{
	OnHoldThresholdReached.Broadcast(inputType, targetMode);
}

void ULRHUDWidgetController::HandleStateChangeRejected(const FLRStateChangeRequest request,
	const FGameplayTag reason)
{
	OnStateChangeRejected.Broadcast(request, reason);
}

/** Stores and forwards the Focus prompt produced by the interaction component. */
void ULRHUDWidgetController::HandleFocusedInteractionChanged(const FLRInteractionFocusSnapshot focusSnapshot)
{
	SourceInteractionFocus = focusSnapshot;
	RefreshInteractionDisplayKey();
}

/** Applies input-layer visibility policy and refreshes the key after a mode transition. */
void ULRHUDWidgetController::HandleInputModeChanged(const ELRInputMode previousMode, const ELRInputMode currentMode)
{
	CurrentInputMode = currentMode;
	RefreshInteractionDisplayKey();
}

/** Refreshes the display after the active hardware family changes. */
void ULRHUDWidgetController::HandleInputHardwareDeviceChanged(
	const FPlatformUserId userId, const FInputDeviceId deviceId)
{
	if (const ALRPlayerController* playerController = ObservedPlayerController.Get())
	{
		if (playerController->GetPlatformUserId() == userId)
		{
			RefreshInteractionDisplayKey();
		}
	}
}

/** Refreshes the display after Enhanced Input rebuilds active mapping contexts. */
void ULRHUDWidgetController::HandleControlMappingsRebuilt()
{
	RefreshInteractionDisplayKey();
}

/** Refreshes the display after user-mappable settings change. */
void ULRHUDWidgetController::HandleUserSettingsChanged(UEnhancedInputUserSettings* settings)
{
	RefreshInteractionDisplayKey();
}

/** Binds the lazily-created settings object and forwards its mapping changes. */
void ULRHUDWidgetController::HandleUserSettingsInitialized(const UEnhancedInputUserSettings* settings)
{
	if (UEnhancedInputUserSettings* oldSettings = EnhancedInputUserSettings.Get())
	{
		oldSettings->OnSettingsChanged.RemoveDynamic(this, &ULRHUDWidgetController::HandleUserSettingsChanged);
	}

	EnhancedInputUserSettings = const_cast<UEnhancedInputUserSettings*>(settings);
	if (UEnhancedInputUserSettings* newSettings = EnhancedInputUserSettings.Get())
	{
		newSettings->OnSettingsChanged.AddDynamic(this, &ULRHUDWidgetController::HandleUserSettingsChanged);
	}
	RefreshInteractionDisplayKey();
}

/** Rebuilds the presentation copy while keeping the gameplay prompt owned by the interaction component. */
void ULRHUDWidgetController::RefreshInteractionDisplayKey()
{
	const FLRInteractionPromptView nextPrompt = BuildInteractionPromptView(SourceInteractionFocus);
	const bool bPromptChanged = !CurrentInteractionPrompt.HasSamePresentationAs(nextPrompt);
	CurrentInteractionPrompt = nextPrompt;
	if (bPromptChanged)
	{
		OnInteractionPromptChanged.Broadcast(CurrentInteractionPrompt);
	}
}

/** Keeps interaction focus data in the interaction domain and adds input presentation only inside the HUD boundary. */
FLRInteractionPromptView ULRHUDWidgetController::BuildInteractionPromptView(
	const FLRInteractionFocusSnapshot& focusSnapshot) const
{
	FLRInteractionPromptView promptView;
	promptView.CopyFocusSnapshot(focusSnapshot);
	promptView.InputAction = ResolvePromptInputAction(focusSnapshot.ActionTag);
	promptView.InputKeyText = CurrentInputMode == ELRInputMode::Gameplay
		? ResolveInteractionDisplayKey(promptView.InputAction)
		: FText::GetEmpty();
	promptView.bVisible = CurrentInputMode == ELRInputMode::Gameplay && focusSnapshot.Target.IsValid();
	return promptView;
}

/** The interaction HUD always reflects the controller's primary interaction semantic rather than a world-owned asset. */
UInputAction* ULRHUDWidgetController::ResolvePromptInputAction(const FGameplayTag actionTag) const
{
	if (!actionTag.IsValid())
	{
		return nullptr;
	}

	const ALRPlayerController* playerController = ObservedPlayerController.Get();
	const ULRInputConfig* inputConfig = playerController ? playerController->GetInputConfig() : nullptr;
	return inputConfig ? inputConfig->InteractAction : nullptr;
}

/** Queries the current active Enhanced Input mappings instead of scanning a default mapping context. */
FText ULRHUDWidgetController::ResolveInteractionDisplayKey(const UInputAction* action) const
{
	const UEnhancedInputLocalPlayerSubsystem* enhancedInput = EnhancedInputSubsystem.Get();
	if (!enhancedInput || !action)
	{
		return FText::GetEmpty();
	}

	const TArray<FKey> mappedKeys = enhancedInput->QueryKeysMappedToAction(action);
	if (mappedKeys.IsEmpty())
	{
		return FText::GetEmpty();
	}

	bool bGamepadActive = false;
	bool bHardwareKnown = false;
	if (const ALRPlayerController* playerController = ObservedPlayerController.Get())
	{
		if (const UInputDeviceSubsystem* inputDeviceSubsystem = InputDeviceSubsystem.Get())
		{
			const FHardwareDeviceIdentifier device = inputDeviceSubsystem->GetMostRecentlyUsedHardwareDevice(
				playerController->GetPlatformUserId());
			bHardwareKnown = device.PrimaryDeviceType != EHardwareDevicePrimaryType::Unspecified;
			bGamepadActive = device.PrimaryDeviceType == EHardwareDevicePrimaryType::Gamepad;
		}
	}

	const FKey* selectedKey = nullptr;
	for (const FKey& key : mappedKeys)
	{
		if (!key.IsValid())
		{
			continue;
		}
		if (bHardwareKnown && key.IsGamepadKey() == bGamepadActive)
		{
			selectedKey = &key;
			break;
		}
		if (!selectedKey && !key.IsGamepadKey())
		{
			selectedKey = &key;
		}
	}

	return selectedKey ? FormatInteractionKey(*selectedKey) : FText::GetEmpty();
}

/** Keeps gameplay data as FKey while applying the current Xbox-style text policy at the UI boundary. */
FText ULRHUDWidgetController::FormatInteractionKey(const FKey& key) const
{
	if (key == EKeys::Gamepad_FaceButton_Left)
	{
		return NSLOCTEXT("LostRunic|HUD", "InteractionKey.GamepadFaceLeft", "X");
	}
	if (key == EKeys::Gamepad_FaceButton_Bottom)
	{
		return NSLOCTEXT("LostRunic|HUD", "InteractionKey.GamepadFaceBottom", "A");
	}
	if (key == EKeys::Gamepad_FaceButton_Right)
	{
		return NSLOCTEXT("LostRunic|HUD", "InteractionKey.GamepadFaceRight", "B");
	}
	if (key == EKeys::Gamepad_FaceButton_Top)
	{
		return NSLOCTEXT("LostRunic|HUD", "InteractionKey.GamepadFaceTop", "Y");
	}
	return key.GetDisplayName();
}

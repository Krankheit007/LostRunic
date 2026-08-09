/**
 * @file LRPlayerController.cpp
 * @brief 绑定 Enhanced Input 语义，把眼部、移动、交互、快捷栏、对话、菜单和过场输入路由到对应组件，并在上下文切换时抑制仍按住的按键。
 *
 * 关联文件：LRPlayerController.h；所属领域：Framework。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Framework/LRPlayerController.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRProjectSettings.h"
#include "Framework/LRCharacter.h"
#include "Gameplay/LRLocomotionComponent.h"
#include "Input/LRInputConfig.h"
#include "Interaction/LRInteractionComponent.h"
#include "Items/LRInventoryComponent.h"
#include "State/LRStateComponent.h"
#include "UI/LRPlayerUIComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ALRPlayerController::ALRPlayerController()
{
	PlayerUI = CreateDefaultSubobject<ULRPlayerUIComponent>(TEXT("PlayerUI"));
}

/**
 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
 */
void ALRPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!InputConfig)
	{
		InputConfig = GetDefault<ULRProjectSettings>()->InputConfig.LoadSynchronous();
	}
	if (PlayerUI)
	{
		PlayerUI->InitializeUI(this);
	}
	SetLRInputMode(InputMode);
}

/**
 * @brief 处理 On Possess 事件，将引擎回调转换为对应领域状态更新。
 * @param pawn 参与本次操作的运行时对象 `pawn`；函数会检查空值和所需接口。
 */
void ALRPlayerController::OnPossess(APawn* pawn)
{
	Super::OnPossess(pawn);
	if (PlayerUI)
	{
		PlayerUI->SetObservedCharacter(Cast<ALRCharacter>(pawn));
	}
}

/**
 * @brief 绑定 PlayerController 使用的 Enhanced Input Action；具体按键仍由 Input Mapping Context 资产决定。
 */
void ALRPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!InputConfig)
	{
		InputConfig = GetDefault<ULRProjectSettings>()->InputConfig.LoadSynchronous();
	}
	UEnhancedInputComponent* enhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	FString inputError;
	const bool bInputValid = InputConfig && InputConfig->Validate(inputError);
	if (!ensureMsgf(enhancedInput && bInputValid, TEXT("%s has invalid input configuration: %s"),
		*GetNameSafe(this), *inputError))
	{
		return;
	}

	enhancedInput->BindAction(InputConfig->MoveAction, ETriggerEvent::Triggered, this, &ALRPlayerController::HandleMove);
	enhancedInput->BindAction(InputConfig->ToggleCrouchAction, ETriggerEvent::Started, this, &ALRPlayerController::HandleSneakToggle);
	enhancedInput->BindAction(InputConfig->RunAction, ETriggerEvent::Started, this, &ALRPlayerController::HandleRunStarted);
	enhancedInput->BindAction(InputConfig->RunAction, ETriggerEvent::Completed, this, &ALRPlayerController::HandleRunStopped);
	enhancedInput->BindAction(InputConfig->RunAction, ETriggerEvent::Canceled, this, &ALRPlayerController::HandleRunStopped);
	enhancedInput->BindAction(InputConfig->CloseEyesAction, ETriggerEvent::Started, this, &ALRPlayerController::HandleCloseEyesStarted);
	enhancedInput->BindAction(InputConfig->CloseEyesAction, ETriggerEvent::Completed, this, &ALRPlayerController::HandleCloseEyesStopped);
	enhancedInput->BindAction(InputConfig->CloseEyesAction, ETriggerEvent::Canceled, this, &ALRPlayerController::HandleCloseEyesStopped);
	enhancedInput->BindAction(InputConfig->OpenEyesAction, ETriggerEvent::Started, this, &ALRPlayerController::HandleOpenEyesStarted);
	enhancedInput->BindAction(InputConfig->OpenEyesAction, ETriggerEvent::Completed, this, &ALRPlayerController::HandleOpenEyesStopped);
	enhancedInput->BindAction(InputConfig->OpenEyesAction, ETriggerEvent::Canceled, this, &ALRPlayerController::HandleOpenEyesStopped);
	enhancedInput->BindAction(InputConfig->InteractAction, ETriggerEvent::Started, this, &ALRPlayerController::HandleInteract);
	enhancedInput->BindAction(InputConfig->QuickSlotActions[0], ETriggerEvent::Started, this, &ALRPlayerController::HandleQuickSlot1);
	enhancedInput->BindAction(InputConfig->QuickSlotActions[1], ETriggerEvent::Started, this, &ALRPlayerController::HandleQuickSlot2);
	enhancedInput->BindAction(InputConfig->QuickSlotActions[2], ETriggerEvent::Started, this, &ALRPlayerController::HandleQuickSlot3);
	enhancedInput->BindAction(InputConfig->QuickSlotActions[3], ETriggerEvent::Started, this, &ALRPlayerController::HandleQuickSlot4);
	enhancedInput->BindAction(InputConfig->UseQuickSlotAction, ETriggerEvent::Started, this, &ALRPlayerController::HandleUseSelectedQuickSlot);
	enhancedInput->BindAction(InputConfig->PreviousQuickSlotAction, ETriggerEvent::Started, this, &ALRPlayerController::HandlePreviousQuickSlot);
	enhancedInput->BindAction(InputConfig->NextQuickSlotAction, ETriggerEvent::Started, this, &ALRPlayerController::HandleNextQuickSlot);
	enhancedInput->BindAction(InputConfig->ConfirmAction, ETriggerEvent::Started, this, &ALRPlayerController::HandleConfirm);
	enhancedInput->BindAction(InputConfig->CancelAction, ETriggerEvent::Started, this, &ALRPlayerController::HandleCancel);
	enhancedInput->BindAction(InputConfig->OpenJournalAction, ETriggerEvent::Started, this, &ALRPlayerController::HandleOpenJournal);
	enhancedInput->BindAction(InputConfig->PauseAction, ETriggerEvent::Started, this, &ALRPlayerController::HandlePause);
}

/**
 * @brief 更新 LRInput Mode，并在需要时同步组件状态或广播变化事件。
 * @param newMode 本次操作使用的 `newMode` 枚举或模式值。
 */
void ALRPlayerController::SetLRInputMode(const ELRInputMode newMode)
{
	const ELRInputMode previousMode = InputMode;
	InputMode = newMode;
	UpdateStateInputBlocker(previousMode, newMode);
	UEnhancedInputLocalPlayerSubsystem* subsystem = GetLocalPlayer()
		? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()) : nullptr;
	UInputMappingContext* context = ResolveContext(newMode);
	if (!subsystem || !context)
	{
		UE_LOG(LogLostRunicUI, Warning, TEXT("Controller=%s cannot apply input mode=%d; missing mapping context."),
			*GetNameSafe(this), static_cast<int32>(newMode));
		return;
	}

	subsystem->ClearAllMappings();
	FModifyContextOptions options;
	options.bIgnoreAllPressedKeysUntilRelease = true;
	subsystem->AddMappingContext(context, static_cast<int32>(newMode), options);
	ConfigureViewportInput(newMode);
	if (previousMode != newMode)
	{
		OnInputModeChanged.Broadcast(previousMode, newMode);
	}
}

/**
 * @brief 处理 Handle Move 事件，将引擎回调转换为对应领域状态更新。
 * @param value 本次输入、状态更新或测试使用的值。
 */
void ALRPlayerController::HandleMove(const FInputActionValue& value)
{
	if (ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		character->ApplyMoveInput(value.Get<FVector2D>());
	}
}

/**
 * @brief 处理 Handle Sneak Toggle 事件，将引擎回调转换为对应领域状态更新。
 */
void ALRPlayerController::HandleSneakToggle()
{
	if (ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		character->GetLocomotionComponent()->ToggleSneak();
	}
}

/**
 * @brief 处理 Handle Run Started 事件，将引擎回调转换为对应领域状态更新。
 */
void ALRPlayerController::HandleRunStarted()
{
	if (ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		character->GetLocomotionComponent()->StartRun();
	}
}

/**
 * @brief 处理 Handle Run Stopped 事件，将引擎回调转换为对应领域状态更新。
 */
void ALRPlayerController::HandleRunStopped()
{
	if (ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		character->GetLocomotionComponent()->StopRun();
	}
}

/**
 * @brief 处理 Handle Close Eyes Started 事件，将引擎回调转换为对应领域状态更新。
 */
void ALRPlayerController::HandleCloseEyesStarted()
{
	if (const ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		character->GetStateComponent()->BeginEyeInput(ELRStateRequestType::CloseEyes);
	}
}

/**
 * @brief 处理 Handle Close Eyes Stopped 事件，将引擎回调转换为对应领域状态更新。
 */
void ALRPlayerController::HandleCloseEyesStopped()
{
	if (const ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		character->GetStateComponent()->EndEyeInput(ELRStateRequestType::CloseEyes);
	}
}

/**
 * @brief 处理 Handle Open Eyes Started 事件，将引擎回调转换为对应领域状态更新。
 */
void ALRPlayerController::HandleOpenEyesStarted()
{
	if (const ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		character->GetStateComponent()->BeginEyeInput(ELRStateRequestType::OpenEyes);
	}
}

/**
 * @brief 处理 Handle Open Eyes Stopped 事件，将引擎回调转换为对应领域状态更新。
 */
void ALRPlayerController::HandleOpenEyesStopped()
{
	if (const ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		character->GetStateComponent()->EndEyeInput(ELRStateRequestType::OpenEyes);
	}
}

/**
 * @brief 处理 Handle Interact 事件，将引擎回调转换为对应领域状态更新。
 */
void ALRPlayerController::HandleInteract()
{
	if (const ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		character->GetInteractionComponent()->PerformPrimaryInteraction();
	}
}

/**
 * @brief 处理 Handle Quick Slot1 事件，将引擎回调转换为对应领域状态更新。
 */
void ALRPlayerController::HandleQuickSlot1() { UseQuickSlot(0); }
/**
 * @brief 处理 Handle Quick Slot2 事件，将引擎回调转换为对应领域状态更新。
 */
void ALRPlayerController::HandleQuickSlot2() { UseQuickSlot(1); }
/**
 * @brief 处理 Handle Quick Slot3 事件，将引擎回调转换为对应领域状态更新。
 */
void ALRPlayerController::HandleQuickSlot3() { UseQuickSlot(2); }
/**
 * @brief 处理 Handle Quick Slot4 事件，将引擎回调转换为对应领域状态更新。
 */
void ALRPlayerController::HandleQuickSlot4() { UseQuickSlot(3); }

/**
 * @brief 处理 Handle Use Selected Quick Slot 事件，将引擎回调转换为对应领域状态更新。
 */
void ALRPlayerController::HandleUseSelectedQuickSlot()
{
	if (const ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		UseQuickSlot(character->GetInventoryComponent()->GetSelectedQuickSlot());
	}
}

/**
 * @brief 处理 Handle Previous Quick Slot 事件，将引擎回调转换为对应领域状态更新。
 */
void ALRPlayerController::HandlePreviousQuickSlot()
{
	if (const ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		character->GetInventoryComponent()->SelectAdjacentQuickSlot(-1);
	}
}

/**
 * @brief 处理 Handle Next Quick Slot 事件，将引擎回调转换为对应领域状态更新。
 */
void ALRPlayerController::HandleNextQuickSlot()
{
	if (const ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		character->GetInventoryComponent()->SelectAdjacentQuickSlot(1);
	}
}

/**
 * @brief 执行 Resolve Context 的纯规则或事务判定，失败时提供结构化原因。
 * @param mode 本次操作使用的 `mode` 枚举或模式值。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
UInputMappingContext* ALRPlayerController::ResolveContext(const ELRInputMode mode) const
{
	if (!InputConfig)
	{
		return nullptr;
	}
	if (mode == ELRInputMode::Dialogue)
	{
		return InputConfig->DialogueContext;
	}
	if (mode == ELRInputMode::Menu)
	{
		return InputConfig->MenuContext;
	}
	if (mode == ELRInputMode::Transition)
	{
		return InputConfig->TransitionContext;
	}
	return InputConfig->GameplayContext;
}

/**
 * @brief 根据最新领域状态刷新 Update State Input Blocker，并仅在值变化时通知订阅者。
 * @param previousMode 本次操作使用的 `previousMode` 枚举或模式值。
 * @param newMode 本次操作使用的 `newMode` 枚举或模式值。
 */
void ALRPlayerController::UpdateStateInputBlocker(const ELRInputMode previousMode, const ELRInputMode newMode)
{
	ALRCharacter* character = Cast<ALRCharacter>(GetPawn());
	ULRStateComponent* state = character ? character->GetStateComponent() : nullptr;
	if (!state)
	{
		return;
	}

	const auto getBlocker = [](const ELRInputMode mode) -> FGameplayTag
	{
		if (mode == ELRInputMode::Dialogue) return LRGameplayTags::StateBlockerDialogue;
		if (mode == ELRInputMode::Menu) return LRGameplayTags::StateBlockerMenu;
		if (mode == ELRInputMode::Transition) return LRGameplayTags::StateBlockerTransition;
		return FGameplayTag();
	};
	state->SetBlockerActive(getBlocker(previousMode), false);
	state->SetBlockerActive(getBlocker(newMode), true);
}

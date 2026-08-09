#include "Framework/LRCharacter.h"

#include "Gameplay/LRLocomotionComponent.h"
#include "Interaction/LRInteractionComponent.h"
#include "Items/LRInventoryComponent.h"
#include "Stealth/LRHideComponent.h"
#include "Stealth/LRNoiseEmitterComponent.h"
#include "State/LRStateComponent.h"
#include "State/LRStatePresentationComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"

ALRCharacter::ALRCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 700.0f;
	CameraBoom->SetRelativeRotation(FRotator(-50.0f, 0.0f, 0.0f));
	CameraBoom->bDoCollisionTest = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	Locomotion = CreateDefaultSubobject<ULRLocomotionComponent>(TEXT("Locomotion"));
	State = CreateDefaultSubobject<ULRStateComponent>(TEXT("State"));
	StatePresentation = CreateDefaultSubobject<ULRStatePresentationComponent>(TEXT("StatePresentation"));
	Inventory = CreateDefaultSubobject<ULRInventoryComponent>(TEXT("Inventory"));
	Interaction = CreateDefaultSubobject<ULRInteractionComponent>(TEXT("Interaction"));
	Hide = CreateDefaultSubobject<ULRHideComponent>(TEXT("Hide"));
	NoiseEmitter = CreateDefaultSubobject<ULRNoiseEmitterComponent>(TEXT("NoiseEmitter"));
	StimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSource"));
	StimuliSource->bAutoRegister = true;
	StimuliSource->RegisterForSense(UAISense_Sight::StaticClass());
}

void ALRCharacter::ApplyMoveInput(const FVector2D& input)
{
	const FRotator controlRotation = Controller ? Controller->GetControlRotation() : FRotator::ZeroRotator;
	const FRotator yawRotation(0.0f, controlRotation.Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(yawRotation).GetUnitAxis(EAxis::X), input.Y);
	AddMovementInput(FRotationMatrix(yawRotation).GetUnitAxis(EAxis::Y), input.X);
}

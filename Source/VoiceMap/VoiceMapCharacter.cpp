// Copyright Epic Games, Inc. All Rights Reserved.

#include "VoiceMapCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Net/VoiceConfig.h" //VOIPTalker 헤더
#include "GameFramework/PlayerState.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AVoiceMapCharacter

AVoiceMapCharacter::AVoiceMapCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// VOIP Talker 컴포넌트를 생성하고, VOIPTalkerComponent 포인터에 할당합니다.
	VOIPTalkerComponent = CreateDefaultSubobject<UVOIPTalker>(TEXT("VOIPTalker"));
}

void AVoiceMapCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	// VOIP 초기화 작업을 호출합니다.
	InitializeVOIP();
}

void AVoiceMapCharacter::SetMicThreshold(float Threshold)
{
	if (VOIPTalkerComponent)
	{
		UVOIPStatics::SetMicThreshold(Threshold);
	}
}

void AVoiceMapCharacter::RegisterWithPlayerState()
{
	if (VOIPTalkerComponent && GetPlayerState())
	{
		VOIPTalkerComponent->RegisterWithPlayerState(GetPlayerState());
	}
}

bool AVoiceMapCharacter::IsLocallyControlled() const
{
	return IsPlayerControlled();
}

void AVoiceMapCharacter::InitializeVOIP()
{
	if (VOIPTalkerComponent)
	{
		// VOIPTalkerComponent가 유효한지 확인합니다.
		if (IsValid(VOIPTalkerComponent))
		{
			// 플레이어 상태에 VOIPTalker를 등록합니다.
			RegisterWithPlayerState();

			// 마이크 임계값을 설정합니다.
			SetMicThreshold(-1.0f);

			// 로컬 플레이어가 제어 중일 때만 VOIP 관련 설정을 진행합니다.
			if (IsLocallyControlled())
			{
				// 콘솔 명령을 실행하여 VOIP를 활성화합니다.
				APlayerController* PlayerController = Cast<APlayerController>(GetController());
				if (PlayerController)
				{
					PlayerController->ConsoleCommand("OSS.VoiceLoopback 1");
				}
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
// Input

void AVoiceMapCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AVoiceMapCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AVoiceMapCharacter::Look);

	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AVoiceMapCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AVoiceMapCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}
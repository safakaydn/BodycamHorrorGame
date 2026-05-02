// Copyright belongs to Real Interactive Studio, 2025


#include "BodycamCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "BodycamShakeComponent.h"
#include "FlashlightComponent.h"
#include "AimOffsetComponent.h"

// Sets default values
ABodycamCharacter::ABodycamCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// We rotate with mouse, not with movement
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false; // Character moves in the direction of input...

	// Pivot -> Camera (so we can add subtle motion later)
	FPCameraPivot = CreateDefaultSubobject<USceneComponent>(TEXT("FPCameraPivot"));
	FPCameraPivot->SetupAttachment(RootComponent);
	FPCameraPivot->SetRelativeLocation(FVector(0.f, 0.f, 64.f)); // Eye height

	// Create a CameraComponent
	FPCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FPCamera"));
	FPCamera->SetupAttachment(FPCameraPivot);
	FPCamera->bUsePawnControlRotation = true; // Rotate the camera with the controller
	FPCamera->SetFieldOfView(90.f);

	// Start with your preferred base FOV
	FPCamera->SetFieldOfView(BaseFOV);

	// Components
	InteractionComp = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComp"));
	BodycamShake = CreateDefaultSubobject<UBodycamShakeComponent>(TEXT("BodycamShake"));
	AimOffset = CreateDefaultSubobject<UAimOffsetComponent>(TEXT("AimOffset"));

	// Flashlight is a SceneComponent — attach it to the camera so it follows
	// the player's view, then the component manages the spotlight itself.
	Flashlight = CreateDefaultSubobject<UFlashlightComponent>(TEXT("Flashlight"));
	Flashlight->SetupAttachment(FPCamera);

	// Default walk speed
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

// Called when the game starts or when spawned
void ABodycamCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Add the mapping context
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (IMC_Player) Sub->AddMappingContext(IMC_Player, 0);
			}
		}
	}

	// Hand the camera pivot to the shake component so it knows what to animate.
	if (BodycamShake && FPCameraPivot)
	{
		BodycamShake->InitializeForPivot(FPCameraPivot);
	}
}

// Called every frame
void ABodycamCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Speed-based FOV (sensor feel)
	if (FPCamera)
	{
		const float SpeedForFOV = GetVelocity().Size2D();
		const float Alpha = FMath::Clamp((SpeedForFOV - WalkSpeed) / FMath::Max(1.f, (SprintSpeed - WalkSpeed)), 0.f, 1.f);
		const float TargetFOV = FMath::Lerp(BaseFOV, SprintFOV, Alpha);
		const float NewFOV = FMath::FInterpTo(FPCamera->FieldOfView, TargetFOV, DeltaTime, FOVInterpSpeed);
		FPCamera->SetFieldOfView(NewFOV);
	}
}


// Called to bind functionality to input
void ABodycamCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
{
	// Jump (fire once)
	if (JumpAction)
	{
		EIC->BindAction(JumpAction, ETriggerEvent::Started,   this, &ACharacter::Jump);
		EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	}

	// Move / Look (yours)
	if (MoveAction) EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABodycamCharacter::Move);
	if (LookAction) EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABodycamCharacter::Look);

	// Sprint
	if (SprintAction)
	{
		EIC->BindAction(SprintAction, ETriggerEvent::Started,   this, &ABodycamCharacter::StartSprint);
		EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &ABodycamCharacter::StopSprint);
	}

	// Flashlight
	if (FlashlightAction)
	{
		EIC->BindAction(FlashlightAction, ETriggerEvent::Started, this, &ABodycamCharacter::ToggleFlashlight);
	}

	// Interact
	if (InteractAction)
	{
		EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &ABodycamCharacter::Interact);
	}
}

}

void ABodycamCharacter::Move(const FInputActionValue& Val)
{
	const FVector2D Ax = Val.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), Ax.Y);
	AddMovementInput(GetActorRightVector(),   Ax.X);
}


void ABodycamCharacter::Look(const FInputActionValue& Val)
{
	const FVector2D Ax = Val.Get<FVector2D>();

	// Route raw look input through the shared aim offset.
	// Whatever the offset doesn't consume comes back as leftover for the camera.
	float LeftoverYaw = Ax.X;
	float LeftoverPitch = Ax.Y;
	if (AimOffset)
	{
		AimOffset->ConsumeLookInput(Ax.X, Ax.Y, LeftoverYaw, LeftoverPitch);
	}

	// Apply leftover to the camera, scaled by camera sensitivity.
	const float CamYawDeltaDeg = LeftoverYaw * CameraYawDegPerInput;
	const float CamPitchDeltaDeg = LeftoverPitch * CameraPitchDegPerInput;

	AddControllerYawInput(CamYawDeltaDeg);
	AddControllerPitchInput(-CamPitchDeltaDeg); // negate so mouse-up looks up
}



/*Sprinting and Flashlight Implementations*/

void ABodycamCharacter::StartSprint(const FInputActionValue& /*Value*/)
{
	bIsSprinting = true;
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void ABodycamCharacter::StopSprint(const FInputActionValue& /*Value*/)
{
	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void ABodycamCharacter::ToggleFlashlight(const FInputActionValue& /*Value*/)
{
	if (Flashlight) Flashlight->ToggleLight();
}


void ABodycamCharacter::Interact(const FInputActionValue& Value)
{
    // Tell the component to fire the raycast
    if (InteractionComp)
    {
        InteractionComp->FireInteraction();
    }
}
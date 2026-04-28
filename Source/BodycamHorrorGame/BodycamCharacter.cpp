// Copyright belongs to Real Interactive Studio, 2025


#include "BodycamCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Components/SpotLightComponent.h"
#include "BodycamShakeComponent.h"

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

	// Flashlight attached to the CAMERA so it follows mouse exactly
	Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
	Flashlight->SetupAttachment(FPCamera);
	Flashlight->SetRelativeLocation(FVector(10.f, 8.f, 0.f)); // small offset to avoid clipping
	Flashlight->SetRelativeRotation(FRotator::ZeroRotator);
	Flashlight->Intensity = 4500.f;
	Flashlight->InnerConeAngle = 16.f;
	Flashlight->OuterConeAngle = 24.f;
	Flashlight->AttenuationRadius = 2600.f;
	Flashlight->bUseInverseSquaredFalloff = false; // more "gamey" falloff
	Flashlight->SetVisibility(false);


	InteractionComp = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComp"));
	BodycamShake = CreateDefaultSubobject<UBodycamShakeComponent>(TEXT("BodycamShake"));

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

	// --- FLASHLIGHT: movement sway & jump/land impulses ---
	UCharacterMovementComponent* Move = GetCharacterMovement();
	const bool bGrounded = Move ? Move->IsMovingOnGround() : true;

	const FVector V = GetVelocity();
	const float   Speed = V.Size2D();
	const float   MaxSpd = Move ? FMath::Max(1.f, Move->MaxWalkSpeed) : 600.f;
	const float   SpeedAlpha = FMath::Clamp(Speed / MaxSpd, 0.f, 1.f);

	// forward/strafe direction (−1..+1) from velocity
	const FVector2D v2(V.X, V.Y);
	const FVector2D f2(GetActorForwardVector().X, GetActorForwardVector().Y);
	const FVector2D r2(GetActorRightVector().X,   GetActorRightVector().Y);
	const FVector2D vN = v2.IsNearlyZero() ? FVector2D::ZeroVector : v2.GetSafeNormal();
	const float fwdNorm    = FVector2D::DotProduct(vN, f2.GetSafeNormal()); // forward/back
	const float strafeNorm = FVector2D::DotProduct(vN, r2.GetSafeNormal()); // left/right

	// sprint amplifies sway a bit
	const float sprintScale = bIsSprinting ? FlashSprintSwayScale : 1.f;

	// target sway from movement + a touch of bob tied to your BobTime
	const float phase = (BodycamShake ? BodycamShake->GetBobTime() : 0.f) * 2.f * PI;
	const float targetMoveYaw   = (strafeNorm * FlashMoveYawByStrafe   * SpeedAlpha * sprintScale)
								+ (FMath::Sin(phase * 0.5f) * FlashBobYaw   * SpeedAlpha);
	const float targetMovePitch = (-fwdNorm   * FlashMovePitchBySpeed  * SpeedAlpha * sprintScale) // dip when moving fast
								+ (FMath::Sin(phase)        * FlashBobPitch * SpeedAlpha);

	// smooth the sway so it doesn’t jitter
	FlashMoveYaw   = FMath::FInterpTo(FlashMoveYaw,   targetMoveYaw,   DeltaTime, FlashMoveInterp);
	FlashMovePitch = FMath::FInterpTo(FlashMovePitch, targetMovePitch, DeltaTime, FlashMoveInterp);

	// jump/land impulses
	if (bFlashPrevGrounded && !bGrounded)  FlashKickAccumPitch += FlashJumpPitchKick; // takeoff
	if (!bFlashPrevGrounded && bGrounded)  FlashKickAccumPitch += FlashLandPitchKick; // landing thud
	bFlashPrevGrounded = bGrounded;
	FlashKickAccumPitch = FMath::FInterpTo(FlashKickAccumPitch, 0.f, DeltaTime, FlashKickReturnSpeed);


	// Free-aim: recentre the offsets when no input, then apply to flashlight
	FlashAimYaw   = FMath::FInterpTo(FlashAimYaw,   0.f, DeltaTime, FlashAimReturnSpeed);
	FlashAimPitch = FMath::FInterpTo(FlashAimPitch, 0.f, DeltaTime, FlashAimReturnSpeed);

	if (Flashlight)
	{
		// your free-aim target (keep your invert toggles)
		const float PitchFA = bInvertFlashAimPitch ? -FlashAimPitch : FlashAimPitch;
		const float YawFA   = bInvertFlashAimYaw   ? -FlashAimYaw   : FlashAimYaw;

		// ADD movement & kicks to free-aim
		const float PitchToApply = PitchFA + FlashMovePitch + FlashKickAccumPitch;
		const float YawToApply   = YawFA   + FlashMoveYaw;
		const FRotator Target(-PitchToApply, YawToApply, 0.f);
		FRotator Cur = Flashlight->GetRelativeRotation();
		Cur.Pitch = FMath::FInterpTo(Cur.Pitch, Target.Pitch, DeltaTime, FlashAimSmoothing);
		Cur.Yaw   = FMath::FInterpTo(Cur.Yaw,   Target.Yaw,   DeltaTime, FlashAimSmoothing);
		Cur.Roll  = 0.f;
		Flashlight->SetRelativeRotation(Cur);
	}

	// Speed-based FOV (sensor feel)
	if (FPCamera)
	{
		const float SpeedForFOV = GetVelocity().Size2D();
		const float alpha = FMath::Clamp((SpeedForFOV - WalkSpeed) / FMath::Max(1.f, (SprintSpeed - WalkSpeed)), 0.f, 1.f);
		const float TargetFOV = FMath::Lerp(BaseFOV, SprintFOV, alpha);
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
	const FVector2D Ax = Val.Get<FVector2D>(); // raw input units from Enhanced Input

    // Split into a "leaked" part (always goes to camera) and the "buffered" part (goes to free-aim first)
    const float LeakYawInput   = Ax.X * FreeAimLeak;
    const float LeakPitchInput = Ax.Y * FreeAimLeak; // positive = mouse up

    const float ProcYawInput   = Ax.X * (1.f - FreeAimLeak);
    const float ProcPitchInput = Ax.Y * (1.f - FreeAimLeak);

    // --- Convert processed input to degrees for the free-aim buffer ---
    const float yawDegIn   = ProcYawInput   * LookToDegYaw;
    const float pitchDegIn = ProcPitchInput * LookToDegPitch;

    // YAW: clamp into [-max, +max]
    const float prevAimYaw = FlashAimYaw;
    FlashAimYaw = FMath::Clamp(FlashAimYaw + yawDegIn, -FlashAimMaxYaw, FlashAimMaxYaw);
    const float usedYawDeg = FlashAimYaw - prevAimYaw;

    // PITCH: clamp into [-max, +max]
    const float prevAimPitch = FlashAimPitch;
    FlashAimPitch = FMath::Clamp(FlashAimPitch + pitchDegIn, -FlashAimMaxPitch, FlashAimMaxPitch);
    const float usedPitchDeg = FlashAimPitch - prevAimPitch;

    // Convert "used for flashlight" back to input units so we know what's left for the camera
    const float yawUnitsUsed   = (LookToDegYaw   > KINDA_SMALL_NUMBER) ? (usedYawDeg   / LookToDegYaw)   : 0.f;
    const float pitchUnitsUsed = (LookToDegPitch > KINDA_SMALL_NUMBER) ? (usedPitchDeg / LookToDegPitch) : 0.f;

    // Leftover input (from the processed part) goes to the camera
    const float leftoverYawInput   = ProcYawInput   - yawUnitsUsed;
    const float leftoverPitchInput = ProcPitchInput - pitchUnitsUsed;

    // Total input to camera = leak + leftover, scaled by CAMERA sensitivity (independent of flashlight)
    const float camYawDeltaDeg   = (LeakYawInput   + leftoverYawInput)   * CameraYawDegPerInput;
    const float camPitchDeltaDeg = (LeakPitchInput + leftoverPitchInput) * CameraPitchDegPerInput;

    // Apply to camera. If you prefer Unreal's "mouse up looks up", keep pitch negative here:
    AddControllerYawInput(camYawDeltaDeg);
    AddControllerPitchInput(-camPitchDeltaDeg); // invert for typical FPS feel


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
	if (Flashlight) Flashlight->ToggleVisibility(true);
}



void ABodycamCharacter::Interact(const FInputActionValue& Value)
{
    // Tell the component to fire the raycast
    if (InteractionComp)
    {
        InteractionComp->FireInteraction();
    }
}
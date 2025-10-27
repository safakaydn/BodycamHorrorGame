// Copyright belongs to Real Interactive Studio, 2025

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BodycamCharacter.generated.h"

struct FInputActionValue;

UCLASS()
class BODYCAMHORRORGAME_API ABodycamCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABodycamCharacter();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	//Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class USceneComponent* FPCameraPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UCameraComponent* FPCamera;


	//Enchanced Input
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* IMC_Player;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	class UInputAction* MoveAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	class UInputAction* LookAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	class UInputAction* JumpAction;

	//Input handlers
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, Category="Bodycam|Headbob")
	float BobIntensity = 0.7f;              // cm up/down when moving

	UPROPERTY(EditAnywhere, Category="Bodycam|Headbob")
	float BobFrequency = 4.5f;              // how fast the bob oscillates

	UPROPERTY(EditAnywhere, Category="Bodycam|Headbob")
	float CrouchBobScale = 0.65f;           // bob reduced while crouched

	UPROPERTY(EditAnywhere, Category="Bodycam|Headbob")
	float SprintBobScale = 1.4f;            // bob amplified at high speed

	UPROPERTY(EditAnywhere, Category="Bodycam|Headbob")
	float BobHorizontal = 0.25f; // Y cm (side sway)

	UPROPERTY(EditAnywhere, Category="Bodycam|Headbob") 
	float BobForward   = 0.35f; // X cm (forward surge)

	UPROPERTY(EditAnywhere, Category="Bodycam|Headbob") 
	float BobPitchDeg  = 0.4f;  // camera nod in degrees

	UPROPERTY(EditAnywhere, Category="Bodycam|Breathing")
	float BreathIntensity = 0.25f;          // cm idle breathing

	UPROPERTY(EditAnywhere, Category="Bodycam|Breathing")
	float BreathXYIntensity = 0.25f;          // cm idle breathing

	UPROPERTY(EditAnywhere, Category="Bodycam|Headbob") 
	float BreathPitchDeg  = 0.4f;  // camera nod in degrees

	UPROPERTY(EditAnywhere, Category="Bodycam|Breathing")
	float BreathFrequency = 1.1f;           // Hz-ish

	UPROPERTY(EditAnywhere, Category="Bodycam|Breathing")
	float BreathRollDeg = 0.25f;          // slight roll with breathing

	UPROPERTY(EditAnywhere, Category="Bodycam|Breathing")
	float BreathNoiseSpeed = 0.9f;

	UPROPERTY(EditAnywhere, Category="Bodycam|Roll")
	float StrafeRollDeg = 2.0f;             // max roll when strafing

	UPROPERTY(EditAnywhere, Category="Bodycam|Roll")
	float RollInterpSpeed = 6.0f;           // smoothing

	UPROPERTY(EditAnywhere, Category="Bodycam|Landing")
	float LandingKick = 3.0f;               // cm downward kick on land

	UPROPERTY(EditAnywhere, Category="Bodycam|Landing")
	float LandingDamp = 12.0f;              // how fast the kick fades

	UPROPERTY(EditAnywhere, Category="Bodycam|Landing") 
	float JumpKickUp = 2.0f; // cm

	UPROPERTY(EditAnywhere, Category="Bodycam|Landing") 
	float JumpDamp   = 10.0f;
	
	float JumpOffset = 0.f;

	// runtime
	float BobTime = 0.f;
	float BreathTime = 0.f;
	float LandingOffset = 0.f;              // positive = push camera down
	bool  bWasGrounded = true;
	FVector PivotBaseRelLoc = FVector::ZeroVector;

	// helper
	void UpdateBodycamPOV(float DeltaSeconds);

	// seeds (initialized once so each pawn drifts differently)
	float BreathSeedX = 0.f, BreathSeedY = 0.f, BreathSeedZ = 0.f, BreathSeedPitch = 0.f, BreathSeedRoll = 0.f;




	/*FLASHLIGHT AND SPRINTING IMPLEMENTATIONS*/


	// ===== Input =====
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
	class UInputAction* SprintAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
	class UInputAction* FlashlightAction = nullptr;

	// ===== Sprint =====
	UPROPERTY(EditAnywhere, Category="Movement|Sprint")
	float WalkSpeed = 250.f;

	UPROPERTY(EditAnywhere, Category="Movement|Sprint")
	float SprintSpeed = 500.f;

	bool bIsSprinting = false;

	// ===== FOV (sensor feel) =====
	UPROPERTY(EditAnywhere, Category="Camera|FOV")
	float BaseFOV = 95.f;

	UPROPERTY(EditAnywhere, Category="Camera|FOV")
	float SprintFOV = 101.f;

	UPROPERTY(EditAnywhere, Category="Camera|FOV")
	float FOVInterpSpeed = 6.f;

	// ===== Flashlight =====
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	class USpotLightComponent* Flashlight = nullptr;

	// --- Flashlight free-aim (beam moves first, then camera) ---
	UPROPERTY(EditAnywhere, Category="Flashlight|FreeAim") float FlashAimMaxYaw   = 10.f; // deg
	UPROPERTY(EditAnywhere, Category="Flashlight|FreeAim") float FlashAimMaxPitch = 8.f;  // deg
	UPROPERTY(EditAnywhere, Category="Flashlight|FreeAim") float FlashAimReturnSpeed = 4.f;   // recenter
	UPROPERTY(EditAnywhere, Category="Flashlight|FreeAim") float FlashAimSmoothing   = 12.f;  // visual smoothing

	// convert look input to degrees that fill the free-aim
	UPROPERTY(EditAnywhere, Category="Flashlight|FreeAim") float LookToDegYaw   = 1.0f;
	UPROPERTY(EditAnywhere, Category="Flashlight|FreeAim") float LookToDegPitch = 1.0f;

	// Flip directions if your mouse/asset axes feel reversed
	UPROPERTY(EditAnywhere, Category="Flashlight|FreeAim")
	bool bInvertFlashAimPitch = true;   // try true first (mouse up -> beam up)

	UPROPERTY(EditAnywhere, Category="Flashlight|FreeAim")
	bool bInvertFlashAimYaw = false;    // usually fine; set true if left/right feels backwards

	// Camera sensitivity (leftover input -> camera), independent of flashlight
	UPROPERTY(EditAnywhere, Category="Flashlight|FreeAim")
	float CameraYawDegPerInput   = 1.0f;   // how many degrees the CAMERA turns per 1.0 input unit (yaw)

	UPROPERTY(EditAnywhere, Category="Flashlight|FreeAim")
	float CameraPitchDegPerInput = 1.0f;   // degrees per 1.0 input unit (pitch)

	// Small fraction of input that always reaches the camera (prevents "stun")
	UPROPERTY(EditAnywhere, Category="Flashlight|FreeAim", meta=(ClampMin="0.0", ClampMax="1.0"))
	float FreeAimLeak = 0.18f;             // 0 = hard deadzone, 0.15~0.30 = soft/comfortable


	// --- Flashlight movement sway (while walking/sprinting) ---
	UPROPERTY(EditAnywhere, Category="Flashlight|Move") 
	float FlashMoveYawByStrafe   = 4.0f; // deg at full strafe
	UPROPERTY(EditAnywhere, Category="Flashlight|Move") 
	float FlashMovePitchBySpeed = 2.5f;  // deg nose-down at full forward speed
	UPROPERTY(EditAnywhere, Category="Flashlight|Move") 
	float FlashSprintSwayScale  = 1.35f; // extra during sprint
	UPROPERTY(EditAnywhere, Category="Flashlight|Move") 
	float FlashBobYaw           = 0.8f;  // tie sway to your bob phase
	UPROPERTY(EditAnywhere, Category="Flashlight|Move") 
	float FlashBobPitch         = 0.5f;
	UPROPERTY(EditAnywhere, Category="Flashlight|Move") 
	float FlashMoveInterp       = 8.0f;  // smoothing

	// --- Flashlight impulses on jump/land ---
	UPROPERTY(EditAnywhere, Category="Flashlight|Jump") 
	float FlashJumpPitchKick   = 4.0f;   // up on takeoff (+)
	UPROPERTY(EditAnywhere, Category="Flashlight|Jump") 
	float FlashLandPitchKick   = -6.0f;  // dip on landing (-)
	UPROPERTY(EditAnywhere, Category="Flashlight|Jump") 
	float FlashKickReturnSpeed = 12.0f;  // decay

	// runtime (no need to expose)
	float FlashMoveYaw = 0.f, FlashMovePitch = 0.f, FlashKickAccumPitch = 0.f;
	bool  bFlashPrevGrounded = true;



	float FlashAimYaw   = 0.f; // current offset relative to camera (deg)
	float FlashAimPitch = 0.f; // current offset relative to camera (deg)

	// Handlers
	void StartSprint(const struct FInputActionValue& Value);
	void StopSprint (const struct FInputActionValue& Value);
	void ToggleFlashlight(const struct FInputActionValue& Value);
	

};

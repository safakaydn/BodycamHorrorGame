// Copyright belongs to Real Interactive Studio, 2025

#pragma once

#include "CoreMinimal.h"
#include "InteractionComponent.h"
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
	void Interact(const FInputActionValue& Value);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Bodycam", meta=(AllowPrivateAccess="true"))
	class UBodycamShakeComponent* BodycamShake = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Bodycam", meta=(AllowPrivateAccess="true"))
	class UFlashlightComponent* Flashlight = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Bodycam", meta=(AllowPrivateAccess="true"))
	class UAimOffsetComponent* AimOffset = nullptr;

	// ===== Pistol =====
	// Visual pistol mesh — the thing the player sees in their view.
	// Attached to the camera in the constructor. Hidden by default until
	// scenario or pickup logic shows it.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pistol", meta=(AllowPrivateAccess="true"))
	class USkeletalMeshComponent* PistolMesh = nullptr;

	// Pistol gameplay logic — ammo, fire rate, equip/holster state.
	// Doesn't own the mesh; orchestrates it.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pistol", meta=(AllowPrivateAccess="true"))
	class UPistolComponent* Pistol = nullptr;
	
	/*INPUT, SPRINT, FOV*/

	// ===== Input =====
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
	class UInputAction* SprintAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
	class UInputAction* FlashlightAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
	class UInputAction* InteractAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction", meta=(AllowPrivateAccess="true"))
	class UInteractionComponent* InteractionComp;

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

	// ===== How aggressively the camera turns per 1.0 unit of leftover input
	// from UAimOffsetComponent. Lives here because it's a camera property,
	// not a flashlight one. =====
	UPROPERTY(EditAnywhere, Category="Camera|Sensitivity")
	float CameraYawDegPerInput = 1.0f;

	UPROPERTY(EditAnywhere, Category="Camera|Sensitivity")
	float CameraPitchDegPerInput = 1.0f;

	// Handlers
	void StartSprint(const struct FInputActionValue& Value);
	void StopSprint(const struct FInputActionValue& Value);
	void ToggleFlashlight(const struct FInputActionValue& Value);
	
	

};

// Copyright belongs to Real Interactive Studio, 2025

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "FlashlightComponent.generated.h"

/**
 * A handheld-feel flashlight component.
 *
 * Owns a USpotLightComponent as a child. Adds three layered motions
 * to the beam direction:
 *  1. AimOffset — read from a UAimOffsetComponent on the owner; the
 *     "I'm holding something" hand-feel.
 *  2. Movement sway — yaw/pitch driven by the owner's velocity and
 *     a phase clock from UBodycamShakeComponent (so the beam syncs
 *     to the body's bob rhythm).
 *  3. Jump/landing kicks — pitch impulses on takeoff and landing
 *     that decay back to neutral.
 *
 * The component itself does not handle input. Whoever owns it
 * (the character) is responsible for calling AimOffsetComponent's
 * ConsumeLookInput() upstream — this component just reads the
 * resulting offset every Tick.
 */
UCLASS(ClassGroup=(Bodycam), meta=(BlueprintSpawnableComponent))
class BODYCAMHORRORGAME_API UFlashlightComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UFlashlightComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Toggle the visual spotlight on/off.
	UFUNCTION(BlueprintCallable, Category="Flashlight")
	void ToggleLight();

	// Explicit setter, for scripted moments (cutscenes forcing flashlight off, etc).
	UFUNCTION(BlueprintCallable, Category="Flashlight")
	void SetLightOn(bool bOn);

	// Whether the spotlight is currently visible.
	UFUNCTION(BlueprintPure, Category="Flashlight")
	bool IsLightOn() const;

protected:
	// The actual visible spotlight. Created in the constructor and attached to us.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Flashlight", meta=(AllowPrivateAccess="true"))
	class USpotLightComponent* Spotlight = nullptr;

	// ---- Movement sway tuning ----
	UPROPERTY(EditAnywhere, Category="Flashlight|Move")
	float MoveYawByStrafe = 4.0f;       // deg at full strafe

	UPROPERTY(EditAnywhere, Category="Flashlight|Move")
	float MovePitchBySpeed = 2.5f;      // deg nose-down at full forward speed

	UPROPERTY(EditAnywhere, Category="Flashlight|Move")
	float SprintSwayScale = 1.35f;      // amplifier while sprinting

	UPROPERTY(EditAnywhere, Category="Flashlight|Move")
	float BobYaw = 0.8f;                // sway tied to body bob phase

	UPROPERTY(EditAnywhere, Category="Flashlight|Move")
	float BobPitch = 0.5f;

	UPROPERTY(EditAnywhere, Category="Flashlight|Move")
	float MoveInterp = 8.0f;            // smoothing of movement-driven sway

	// ---- Jump / land impulses ----
	UPROPERTY(EditAnywhere, Category="Flashlight|Jump")
	float JumpPitchKick = 4.0f;         // up on takeoff (+)

	UPROPERTY(EditAnywhere, Category="Flashlight|Jump")
	float LandPitchKick = -6.0f;        // dip on landing (-)

	UPROPERTY(EditAnywhere, Category="Flashlight|Jump")
	float KickReturnSpeed = 12.0f;      // decay rate

	// ---- Final smoothing ----
	UPROPERTY(EditAnywhere, Category="Flashlight|Smoothing")
	float AimSmoothing = 12.0f;         // visual smoothing of beam rotation

	// ---- Inversion toggles ----
	// If your free-aim feels reversed for a player, flip these.
	UPROPERTY(EditAnywhere, Category="Flashlight|Smoothing")
	bool bInvertAimPitch = true;

	UPROPERTY(EditAnywhere, Category="Flashlight|Smoothing")
	bool bInvertAimYaw = false;

	// ---- Default light parameters (applied in constructor; can be tweaked in editor) ----
	UPROPERTY(EditAnywhere, Category="Flashlight|Light")
	float Intensity = 4500.f;

	UPROPERTY(EditAnywhere, Category="Flashlight|Light")
	float InnerConeAngle = 16.f;

	UPROPERTY(EditAnywhere, Category="Flashlight|Light")
	float OuterConeAngle = 24.f;

	UPROPERTY(EditAnywhere, Category="Flashlight|Light")
	float AttenuationRadius = 2600.f;

	// Whether to use Unreal's physical inverse-square falloff or a flatter "gamey" curve.
	// false = gamey/cone-like; true = physically accurate but reaches less far.
	UPROPERTY(EditAnywhere, Category="Flashlight|Light")
	bool bUseInverseSquaredFalloff = false;

	// Whether the light is on at game start.
	UPROPERTY(EditAnywhere, Category="Flashlight|Light")
	bool bStartLightOn = false;

private:
	// ---- Cached references ----
	// Resolved in BeginPlay by walking the owner's components.
	UPROPERTY()
	class UAimOffsetComponent* AimOffset = nullptr;

	UPROPERTY()
	class UBodycamShakeComponent* BodycamShake = nullptr;

	// ---- Runtime state for movement sway and impulses ----
	float MoveYaw = 0.f;
	float MovePitch = 0.f;
	float KickAccumPitch = 0.f;
	bool  bPrevGrounded = true;
};

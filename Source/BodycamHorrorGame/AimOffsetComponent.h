// Copyright belongs to Real Interactive Studio, 2025

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AimOffsetComponent.generated.h"

/**
 * Holds and manages a 2D angular "aim offset" used by held items
 * (flashlight, pistol) to feel like they're being held rather than
 * rigidly attached to the camera.
 *
 * The component is always active. Input flowing in is split between
 * the offset (clamped to MaxYaw/MaxPitch) and "leftover" that the
 * caller applies to the camera. Each frame, the offset gently returns
 * toward zero so the held item recenters when the player stops moving
 * the mouse.
 *
 * Tune feel via MaxYaw, MaxPitch, LeakFactor, and ReturnSpeed.
 */
UCLASS(ClassGroup=(Bodycam), meta=(BlueprintSpawnableComponent))
class BODYCAMHORRORGAME_API UAimOffsetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAimOffsetComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * Routes raw look input (in Enhanced Input units) between this
	 * offset and the camera. Updates internal state and returns
	 * the leftover that the caller should apply to the camera.
	 *
	 * @param YawInput   raw yaw input from this frame
	 * @param PitchInput raw pitch input from this frame
	 * @param OutLeftoverYaw   how much yaw input the caller should still apply to the camera
	 * @param OutLeftoverPitch how much pitch input the caller should still apply to the camera
	 */
	void ConsumeLookInput(float YawInput, float PitchInput, float& OutLeftoverYaw, float& OutLeftoverPitch);

	// Returns the current offset (yaw, pitch) in degrees, for whoever
	// wants to apply it (e.g., the flashlight component rotating its beam,
	// or the pistol component rotating the gun).
	FVector2D GetCurrentOffsetDegrees() const { return FVector2D(AimYaw, AimPitch); }

protected:
	// ---- Tuning ----
	// Max angular offset from center, in degrees, in either direction.
	UPROPERTY(EditAnywhere, Category="AimOffset|Tuning")
	float MaxYaw = 8.f;

	UPROPERTY(EditAnywhere, Category="AimOffset|Tuning")
	float MaxPitch = 6.f;

	// How many degrees of offset are produced per 1.0 unit of look input.
	// Multiply your input sensitivity by this to choose how aggressively
	// the offset fills toward its max.
	UPROPERTY(EditAnywhere, Category="AimOffset|Tuning")
	float InputToDegYaw = 1.0f;

	UPROPERTY(EditAnywhere, Category="AimOffset|Tuning")
	float InputToDegPitch = 1.0f;

	// Fraction of input that ALWAYS goes through to the camera even when
	// the offset has room to consume more. Prevents "stuck" feel.
	// 0.0 = hard deadzone (offset eats everything until clamped)
	// 0.2-0.3 = soft, comfortable.
	UPROPERTY(EditAnywhere, Category="AimOffset|Tuning", meta=(ClampMin="0.0", ClampMax="1.0"))
	float LeakFactor = 0.18f;

	// How quickly the offset drifts back to zero when no input is given.
	UPROPERTY(EditAnywhere, Category="AimOffset|Tuning")
	float ReturnSpeed = 4.f;

	// How quickly the offset eases toward the target each frame when input
	// is being applied. Higher = snappier / more responsive. Lower = heavier.
	// Works in conjunction with the soft cap (tanh) for a "muscle response"
	// feel — input never causes an instant jump, even at saturation.
	UPROPERTY(EditAnywhere, Category="AimOffset|Tuning")
	float FollowSpeed = 25.f;

private:
	// Current offset state, in degrees, both in [-Max, +Max].
	float AimYaw = 0.f;
	float AimPitch = 0.f;
};
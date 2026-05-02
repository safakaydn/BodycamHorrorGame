// Copyright belongs to Real Interactive Studio, 2025

#include "AimOffsetComponent.h"

UAimOffsetComponent::UAimOffsetComponent()
{
	// We need to tick to gradually return the offset to zero each frame.
	PrimaryComponentTick.bCanEverTick = true;
}

void UAimOffsetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Drift offsets gently back toward zero when the player stops moving the mouse.
	// FInterpTo gives a smooth, framerate-independent ease-out toward the target (0).
	AimYaw   = FMath::FInterpTo(AimYaw,   0.f, DeltaTime, ReturnSpeed);
	AimPitch = FMath::FInterpTo(AimPitch, 0.f, DeltaTime, ReturnSpeed);
}

void UAimOffsetComponent::ConsumeLookInput(float YawInput, float PitchInput, float& OutLeftoverYaw, float& OutLeftoverPitch)
{
	// --- 1) Split incoming input into "leak" (always to camera) and "processed" (offered to offset first) ---
	const float LeakYawInput   = YawInput   * LeakFactor;
	const float LeakPitchInput = PitchInput * LeakFactor;

	const float ProcYawInput   = YawInput   * (1.f - LeakFactor);
	const float ProcPitchInput = PitchInput * (1.f - LeakFactor);

	// --- 2) Convert the processed input portion into degrees ---
	const float YawDegIn   = ProcYawInput   * InputToDegYaw;
	const float PitchDegIn = ProcPitchInput * InputToDegPitch;

	// --- 3) YAW: soft-cap the proposed new offset, then smoothly ease toward it ---
	const float PrevYaw       = AimYaw;
	const float ProposedYaw   = AimYaw + YawDegIn;
	// tanh squashes the proposed value into a smooth S-curve approaching ±MaxYaw.
	// Linear near zero (no felt resistance), gradually firming up near the limits.
	const float SoftTargetYaw = MaxYaw * FMath::Tanh(ProposedYaw / FMath::Max(MaxYaw, KINDA_SMALL_NUMBER));
	// Get DeltaTime from the world so we stay framerate-independent.
	const float DeltaSeconds  = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;
	AimYaw                    = FMath::FInterpTo(AimYaw, SoftTargetYaw, DeltaSeconds, FollowSpeed);
	const float UsedYawDeg    = AimYaw - PrevYaw;

	// --- 4) PITCH: mirror logic ---
	const float PrevPitch       = AimPitch;
	const float ProposedPitch   = AimPitch + PitchDegIn;
	const float SoftTargetPitch = MaxPitch * FMath::Tanh(ProposedPitch / FMath::Max(MaxPitch, KINDA_SMALL_NUMBER));
	AimPitch                    = FMath::FInterpTo(AimPitch, SoftTargetPitch, DeltaSeconds, FollowSpeed);
	const float UsedPitchDeg    = AimPitch - PrevPitch;

	// --- 5) Convert the "used" degrees back into input units, so we know what we couldn't fit ---
	const float YawUnitsUsed   = (InputToDegYaw   > KINDA_SMALL_NUMBER) ? (UsedYawDeg   / InputToDegYaw)   : 0.f;
	const float PitchUnitsUsed = (InputToDegPitch > KINDA_SMALL_NUMBER) ? (UsedPitchDeg / InputToDegPitch) : 0.f;

	// --- 6) Whatever input the offset couldn't consume goes back to the camera ---
	const float LeftoverProcYaw   = ProcYawInput   - YawUnitsUsed;
	const float LeftoverProcPitch = ProcPitchInput - PitchUnitsUsed;

	// --- 7) Total leftover for the camera = leak + leftover from processed ---
	OutLeftoverYaw   = LeakYawInput   + LeftoverProcYaw;
	OutLeftoverPitch = LeakPitchInput + LeftoverProcPitch;
}

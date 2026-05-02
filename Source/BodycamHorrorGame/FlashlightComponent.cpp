// Copyright belongs to Real Interactive Studio, 2025

#include "FlashlightComponent.h"
#include "AimOffsetComponent.h"
#include "BodycamShakeComponent.h"
#include "Components/SpotLightComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UFlashlightComponent::UFlashlightComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Create the visible spotlight as a child of this component.
	// Doing it here (in the constructor) means it's set up as part of the
	// component's default subobjects, visible in the editor's component tree.
	Spotlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Spotlight"));
	Spotlight->SetupAttachment(this);

	// Small forward+side offset so the cone doesn't clip into the camera.
	// Same values as the original character setup.
	Spotlight->SetRelativeLocation(FVector(10.f, 8.f, 0.f));
	Spotlight->SetRelativeRotation(FRotator::ZeroRotator);

	// Default light parameters; can be tweaked per-instance in the editor
	// because they're driven by UPROPERTYs on this component.
	Spotlight->Intensity = Intensity;
	Spotlight->InnerConeAngle = InnerConeAngle;
	Spotlight->OuterConeAngle = OuterConeAngle;
	Spotlight->AttenuationRadius = AttenuationRadius;
	Spotlight->bUseInverseSquaredFalloff = bUseInverseSquaredFalloff;
	Spotlight->SetVisibility(false); // Off by default; BeginPlay applies bStartLightOn
}

void UFlashlightComponent::BeginPlay()
{
	Super::BeginPlay();

	// Apply the start state. We do this here rather than in the constructor so
	// editor-changed values of bStartLightOn take effect at play time.
	if (Spotlight)
	{
		Spotlight->SetVisibility(bStartLightOn);
	}

	// Cache references to the other components on our owner. We do this once,
	// here, instead of every Tick — components are not added or removed at runtime.
	if (AActor* OwnerActor = GetOwner())
	{
		AimOffset = OwnerActor->FindComponentByClass<UAimOffsetComponent>();
		BodycamShake = OwnerActor->FindComponentByClass<UBodycamShakeComponent>();
	}
}

void UFlashlightComponent::ToggleLight()
{
	if (Spotlight)
	{
		Spotlight->ToggleVisibility();
	}
}

void UFlashlightComponent::SetLightOn(bool bOn)
{
	if (Spotlight)
	{
		Spotlight->SetVisibility(bOn);
	}
}

bool UFlashlightComponent::IsLightOn() const
{
	return Spotlight ? Spotlight->IsVisible() : false;
}

void UFlashlightComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// We need a Character owner to read velocity and grounded state.
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar) return;

	UCharacterMovementComponent* Move = OwnerChar->GetCharacterMovement();
	const bool bGrounded = Move ? Move->IsMovingOnGround() : true;

	// --- Movement sway ---
	const FVector Vel = OwnerChar->GetVelocity();
	const float Speed = Vel.Size2D();
	const float MaxSpd = Move ? FMath::Max(1.f, Move->MaxWalkSpeed) : 600.f;
	const float SpeedAlpha = FMath::Clamp(Speed / MaxSpd, 0.f, 1.f);

	// Forward / strafe components from velocity, projected onto the actor's axes.
	const FVector2D V2(Vel.X, Vel.Y);
	const FVector2D F2(OwnerChar->GetActorForwardVector().X, OwnerChar->GetActorForwardVector().Y);
	const FVector2D R2(OwnerChar->GetActorRightVector().X, OwnerChar->GetActorRightVector().Y);
	const FVector2D VN = V2.IsNearlyZero() ? FVector2D::ZeroVector : V2.GetSafeNormal();

	const float FwdNorm = FVector2D::DotProduct(VN, F2.GetSafeNormal());     // -1..+1 forward/back
	const float StrafeNorm = FVector2D::DotProduct(VN, R2.GetSafeNormal()); // -1..+1 left/right

	// Sprint detection: simple speed test against MaxWalkSpeed. We don't have
	// to know whether the character is *intentionally* sprinting; we just amplify
	// sway when actual speed exceeds normal walking.
	const bool bIsSprintingNow = Move ? (Speed > MaxSpd * 0.7f) : false;
	const float SprintScale = bIsSprintingNow ? SprintSwayScale : 1.f;

	// Bob phase from the shake component, if available. Without it, we use 0 and
	// the bob-driven portion of sway just goes silent — graceful fallback.
	const float BobTime = BodycamShake ? BodycamShake->GetBobTime() : 0.f;
	const float Phase = BobTime * 2.f * PI;

	const float TargetMoveYaw =
		(StrafeNorm * MoveYawByStrafe * SpeedAlpha * SprintScale) +
		(FMath::Sin(Phase * 0.5f) * BobYaw * SpeedAlpha);

	const float TargetMovePitch =
		(-FwdNorm * MovePitchBySpeed * SpeedAlpha * SprintScale) +
		(FMath::Sin(Phase) * BobPitch * SpeedAlpha);

	// Smooth the sway so it doesn't jitter on small velocity changes.
	MoveYaw   = FMath::FInterpTo(MoveYaw,   TargetMoveYaw,   DeltaTime, MoveInterp);
	MovePitch = FMath::FInterpTo(MovePitch, TargetMovePitch, DeltaTime, MoveInterp);

	// --- Jump / land impulses ---
	if (bPrevGrounded && !bGrounded)  KickAccumPitch += JumpPitchKick;   // takeoff
	if (!bPrevGrounded && bGrounded)  KickAccumPitch += LandPitchKick;   // landing
	bPrevGrounded = bGrounded;
	KickAccumPitch = FMath::FInterpTo(KickAccumPitch, 0.f, DeltaTime, KickReturnSpeed);

	// --- Aim offset from the shared component ---
	FVector2D AimDeg = FVector2D::ZeroVector;
	if (AimOffset)
	{
		AimDeg = AimOffset->GetCurrentOffsetDegrees(); // X = yaw, Y = pitch
	}

	const float AimYaw   = bInvertAimYaw   ? -AimDeg.X : AimDeg.X;
	const float AimPitch = bInvertAimPitch ? -AimDeg.Y : AimDeg.Y;

	// --- Compose the final beam rotation ---
	const float TotalYaw   = AimYaw   + MoveYaw;
	const float TotalPitch = AimPitch + MovePitch + KickAccumPitch;

	// We negate pitch when constructing the FRotator because Unreal's pitch
	// convention is "positive = nose up," but we built our offsets in the
	// "looking up = positive" sense from input. The sign here matches the
	// original character code's behavior.
	const FRotator Target(-TotalPitch, TotalYaw, 0.f);

	// Smooth the visible rotation, separately on each axis. Roll stays at zero.
	FRotator Cur = GetRelativeRotation();
	Cur.Pitch = FMath::FInterpTo(Cur.Pitch, Target.Pitch, DeltaTime, AimSmoothing);
	Cur.Yaw   = FMath::FInterpTo(Cur.Yaw,   Target.Yaw,   DeltaTime, AimSmoothing);
	Cur.Roll  = 0.f;
	SetRelativeRotation(Cur);
}
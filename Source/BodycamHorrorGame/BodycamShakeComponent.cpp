// Copyright belongs to Real Interactive Studio, 2025

#include "BodycamShakeComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UBodycamShakeComponent::UBodycamShakeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBodycamShakeComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize per-instance random seeds so multiple characters don't
	// breathe in sync. GetUniqueID is deterministic per-pawn instance.
	FRandomStream RS;
	RS.Initialize(GetOwner() ? GetOwner()->GetUniqueID() : 0);
	BreathSeedX     = RS.FRandRange(-1000.f, 1000.f);
	BreathSeedY     = RS.FRandRange(-1000.f, 1000.f);
	BreathSeedZ     = RS.FRandRange(-1000.f, 1000.f);
	BreathSeedPitch = RS.FRandRange(-1000.f, 1000.f);
	BreathSeedRoll  = RS.FRandRange(-1000.f, 1000.f);
}

void UBodycamShakeComponent::InitializeForPivot(USceneComponent* InPivot)
{
	TargetPivot = InPivot;
	if (TargetPivot)
	{
		PivotBaseRelLoc = TargetPivot->GetRelativeLocation();
	}
}

void UBodycamShakeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateShake(DeltaTime);
}

void UBodycamShakeComponent::UpdateShake(float DeltaSeconds)
{
	if (!TargetPivot) return;

	// We need the owning character to read velocity + grounded state.
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar) return;

	UCharacterMovementComponent* Move = OwnerChar->GetCharacterMovement();
	const bool bGrounded = Move ? Move->IsMovingOnGround() : true;

	const FVector Vel = OwnerChar->GetVelocity();
	const float  Speed2D = FVector(Vel.X, Vel.Y, 0.f).Size();

	// --- Breathing (Perlin noise based) ---
	BreathTime += DeltaSeconds;
	const float t = BreathTime * BreathNoiseSpeed;

	const float nx = FMath::PerlinNoise1D(t * 0.87f + BreathSeedX);
	const float ny = FMath::PerlinNoise1D(t * 1.03f + BreathSeedY);
	const float nz = FMath::PerlinNoise1D(t * 1.19f + BreathSeedZ);
	const float np = FMath::PerlinNoise1D(t * 0.77f + BreathSeedPitch);
	const float nr = FMath::PerlinNoise1D(t * 0.93f + BreathSeedRoll);

	// Reduce breathing while moving so it doesn't fight the bob
	float MoveAlpha = 0.f;
	if (Move)
	{
		const float MaxSpd = FMath::Max(1.f, Move->MaxWalkSpeed);
		MoveAlpha = FMath::Clamp(Speed2D / MaxSpd, 0.f, 1.f);
	}
	const float BreathMoveScale = 1.f - 0.6f * MoveAlpha;

	const float BreathX     = nx * BreathXYIntensity * BreathMoveScale;
	const float BreathY     = ny * BreathXYIntensity * BreathMoveScale;
	const float BreathZ     = nz * BreathIntensity   * BreathMoveScale;
	const float BreathPitch = np * BreathPitchDeg    * BreathMoveScale;
	const float BreathRoll  = nr * BreathRollDeg     * BreathMoveScale;

	// --- Bob amplitude shaping ---
	float Scale = 0.f;
	if (bGrounded && Speed2D > 10.f)
	{
		const float SpeedNorm = Move ? FMath::Clamp(Speed2D / FMath::Max(1.f, Move->MaxWalkSpeed), 0.f, 2.f) : 0.f;
		Scale = FMath::Lerp(1.0f, SprintBobScale, FMath::Clamp(SpeedNorm - 0.5f, 0.f, 1.f));
		if (Move && Move->IsCrouching()) Scale *= CrouchBobScale;

		BobTime += DeltaSeconds * BobFrequency;
	}
	else
	{
		BobTime = 0.f;
	}

	// --- Landing / jump kicks ---
	if (bWasGrounded && !bGrounded) JumpOffset    = JumpKickUp;    // takeoff
	if (!bWasGrounded && bGrounded) LandingOffset = LandingKick;   // landing

	bWasGrounded = bGrounded;

	LandingOffset = FMath::FInterpTo(LandingOffset, 0.f, DeltaSeconds, LandingDamp);
	JumpOffset    = FMath::FInterpTo(JumpOffset,    0.f, DeltaSeconds, JumpDamp);

	// --- Bob offsets ---
	const float Phase = BobTime * 2.f * PI;
	const float OffZ  = (bGrounded ? FMath::Sin(Phase)        * (BobIntensity  * Scale) : 0.f);
	const float OffY  = (bGrounded ? FMath::Sin(Phase * 0.5f) * (BobHorizontal * Scale) : 0.f);
	const float OffX  = (bGrounded ? -FMath::Cos(Phase)       * (BobForward    * Scale) : 0.f);

	// --- Apply to pivot (one location update) ---
	const FVector TargetRel = PivotBaseRelLoc + FVector(
		OffX + BreathX,
		OffY + BreathY,
		BreathZ + OffZ - LandingOffset + JumpOffset);

	FVector CurRel = TargetPivot->GetRelativeLocation();
	CurRel = FMath::VInterpTo(CurRel, TargetRel, DeltaSeconds, 10.f);
	TargetPivot->SetRelativeLocation(CurRel);

	// --- Rotation: strafe roll + bob nod + breathing ---
	const float Lateral     = FVector::DotProduct(Vel.GetSafeNormal2D(), OwnerChar->GetActorRightVector());
	const float TargetRoll  = (Lateral * StrafeRollDeg) + BreathRoll;
	const float TargetPitch = FMath::Sin(Phase + PI * 0.5f) * (BobPitchDeg * Scale) + BreathPitch;

	FRotator R = TargetPivot->GetRelativeRotation();
	R.Roll  = FMath::FInterpTo(R.Roll,  TargetRoll,  DeltaSeconds, RollInterpSpeed);
	R.Pitch = FMath::FInterpTo(R.Pitch, TargetPitch, DeltaSeconds, 6.f);
	R.Yaw   = 0.f;
	TargetPivot->SetRelativeRotation(R);
}
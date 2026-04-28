// Copyright belongs to Real Interactive Studio, 2025

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BodycamShakeComponent.generated.h"

UCLASS(ClassGroup=(Bodycam), meta=(BlueprintSpawnableComponent))
class BODYCAMHORRORGAME_API UBodycamShakeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBodycamShakeComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Called by the owning character to tell us which scene component to shake.
	// We don't own the pivot — the character does. We just animate it.
	void InitializeForPivot(class USceneComponent* InPivot);

	// Read the current bob phase clock. Other systems (like the flashlight)
	// can use this to sync their motion to the body's bob rhythm.
	float GetBobTime() const { return BobTime; }

protected:
	virtual void BeginPlay() override;

private:
	// The pivot we're shaking (owned by the character, not by us)
	UPROPERTY()
	class USceneComponent* TargetPivot = nullptr;

	// ---- Headbob ----
	UPROPERTY(EditAnywhere, Category="Bodycam|Headbob")
	float BobIntensity = 0.7f;

	UPROPERTY(EditAnywhere, Category="Bodycam|Headbob")
	float BobFrequency = 4.5f;

	UPROPERTY(EditAnywhere, Category="Bodycam|Headbob")
	float CrouchBobScale = 0.65f;

	UPROPERTY(EditAnywhere, Category="Bodycam|Headbob")
	float SprintBobScale = 1.4f;

	UPROPERTY(EditAnywhere, Category="Bodycam|Headbob")
	float BobHorizontal = 0.25f;

	UPROPERTY(EditAnywhere, Category="Bodycam|Headbob")
	float BobForward = 0.35f;

	UPROPERTY(EditAnywhere, Category="Bodycam|Headbob")
	float BobPitchDeg = 0.4f;

	// ---- Breathing ----
	UPROPERTY(EditAnywhere, Category="Bodycam|Breathing")
	float BreathIntensity = 0.25f;

	UPROPERTY(EditAnywhere, Category="Bodycam|Breathing")
	float BreathXYIntensity = 0.25f;

	UPROPERTY(EditAnywhere, Category="Bodycam|Breathing")
	float BreathPitchDeg = 0.4f;

	UPROPERTY(EditAnywhere, Category="Bodycam|Breathing")
	float BreathFrequency = 1.1f;

	UPROPERTY(EditAnywhere, Category="Bodycam|Breathing")
	float BreathRollDeg = 0.25f;

	UPROPERTY(EditAnywhere, Category="Bodycam|Breathing")
	float BreathNoiseSpeed = 0.9f;

	// ---- Roll (lean while strafing) ----
	UPROPERTY(EditAnywhere, Category="Bodycam|Roll")
	float StrafeRollDeg = 2.0f;

	UPROPERTY(EditAnywhere, Category="Bodycam|Roll")
	float RollInterpSpeed = 6.0f;

	// ---- Landing / jump kicks ----
	UPROPERTY(EditAnywhere, Category="Bodycam|Landing")
	float LandingKick = 3.0f;

	UPROPERTY(EditAnywhere, Category="Bodycam|Landing")
	float LandingDamp = 12.0f;

	UPROPERTY(EditAnywhere, Category="Bodycam|Landing")
	float JumpKickUp = 2.0f;

	UPROPERTY(EditAnywhere, Category="Bodycam|Landing")
	float JumpDamp = 10.0f;

	// ---- Runtime state ----
	float BobTime = 0.f;
	float BreathTime = 0.f;
	float LandingOffset = 0.f;
	float JumpOffset = 0.f;
	bool  bWasGrounded = true;
	FVector PivotBaseRelLoc = FVector::ZeroVector;

	// Per-instance random seeds so two characters don't breathe in sync
	float BreathSeedX = 0.f;
	float BreathSeedY = 0.f;
	float BreathSeedZ = 0.f;
	float BreathSeedPitch = 0.f;
	float BreathSeedRoll = 0.f;

	// Helper that does the actual math each frame
	void UpdateShake(float DeltaSeconds);
};
// Copyright belongs to Real Interactive Studio, 2025

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interactable.h" // Include our new interface
#include "InteractionComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BODYCAMHORRORGAME_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInteractionComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Call this when Player presses E
	void FireInteraction();

	// --- Variables for Animation Blueprint (Active Hands) ---
	
	// Is the player currently looking at something interactable?
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	bool bIsFocusingInteractable;

	// Where should the hand reach towards? (World Space)
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FVector HandTargetLocation;

protected:
	virtual void BeginPlay() override;

private:
	void PerformInteractionCheck();

	UPROPERTY(EditAnywhere, Category = "Interaction")
	float InteractionDistance = 220.0f; // Roughly arm's length

	// The object we are currently looking at
	AActor* FocusedActor;
};
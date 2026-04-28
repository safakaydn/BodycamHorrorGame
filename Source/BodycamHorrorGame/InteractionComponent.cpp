// Copyright belongs to Real Interactive Studio, 2025


#include "InteractionComponent.h"
#include "BodycamCharacter.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	PerformInteractionCheck();
}

void UInteractionComponent::PerformInteractionCheck()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	FVector EyeLoc;
	FRotator EyeRot;
	Owner->GetActorEyesViewPoint(EyeLoc, EyeRot);

	FVector End = EyeLoc + (EyeRot.Vector() * InteractionDistance);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);

	// Perform Line Trace
	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, EyeLoc, End, ECC_Visibility, Params);

	if (bHit && Hit.GetActor() && Hit.GetActor()->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
	{
		FocusedActor = Hit.GetActor();
		bIsFocusingInteractable = true;

		// ASK the object where the hand should go (Interface Call)
		// If it's a door, it returns the knob location. If it's a key, it returns the key location.
		HandTargetLocation = IInteractable::Execute_GetInteractLocation(FocusedActor);
	}
	else
	{
		FocusedActor = nullptr;
		bIsFocusingInteractable = false;
		HandTargetLocation = FVector::ZeroVector; // Or reset to a neutral spot
	}
}

void UInteractionComponent::FireInteraction()
{
	if (FocusedActor)
	{
		IInteractable::Execute_Interact(FocusedActor, Cast<ABodycamCharacter>(GetOwner()));
	}
}


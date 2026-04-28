#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

// Enum to define interaction types (Simple click, Hold, Inspection)
UENUM(BlueprintType)
enum class EInteractMode : uint8
{
	EIM_Simple      UMETA(DisplayName = "Simple Click"),
	EIM_Inspect     UMETA(DisplayName = "Inspect Item"),
	EIM_DoorContext UMETA(DisplayName = "Door Context"),
	EIM_Forced      UMETA(DisplayName = "Forced Sequence")
};

UINTERFACE(MinimalAPI)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

class BODYCAMHORRORGAME_API IInteractable
{
	GENERATED_BODY()

public:
	// Main interaction event
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(class ABodycamCharacter* Character);

	// Returns the location where the hand should reach (e.g., the Door Knob location)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FVector GetInteractLocation();

	// Returns what kind of interaction this is (so the player knows whether to inspect or just click)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	EInteractMode GetInteractMode();
};
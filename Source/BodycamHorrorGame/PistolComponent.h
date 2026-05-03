// Copyright belongs to Real Interactive Studio, 2025

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/HitResult.h"
#include "PistolComponent.generated.h"



// --- Delegates ---
//
// These are how the component talks to the rest of the game. When something
// happens inside the pistol (a shot fires, the mag goes empty, a reload
// finishes), the component broadcasts the matching delegate. Any code that
// cares — the character, the AnimBP, the HUD, gameplay scripts — subscribes
// to these and reacts.
//
// We use DYNAMIC delegates because they're Blueprint-assignable, which lets
// the eventual BP_BodycamCharacter (and the asset's AnimBP) bind to them
// directly from the editor without writing more C++.

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPistolFired, const FHitResult&, HitResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPistolDryFire);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPistolEquipped);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPistolHolstered);

/**
 * Logic-only pistol component.
 *
 * Owns the gameplay state of the pistol — ammo, fire rate, equipped state,
 * the scenario "can fire now" gate. Does NOT own the pistol mesh; that lives
 * as a separate USkeletalMeshComponent on the owning character. Does NOT
 * play animations directly; it broadcasts events that the AnimBP listens to.
 *
 * Reads aim direction from a sibling UAimOffsetComponent on the owner.
 *
 * Public API: StartFire, StopFire, Reload, Equip, Holster, SetCanFire.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BODYCAMHORRORGAME_API UPistolComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPistolComponent();

	// --- Public API: gameplay actions ---

	/** Pull the trigger. Fires one round if all conditions are met. */
	UFUNCTION(BlueprintCallable, Category="Pistol")
	void StartFire();

	/** Release the trigger. (Reserved for future full-auto support.) */
	UFUNCTION(BlueprintCallable, Category="Pistol")
	void StopFire();

	/** Begin a reload. No-op if already reloading, mag is full, or reserve is empty. */
	UFUNCTION(BlueprintCallable, Category="Pistol")
	void Reload();

	/** Equip the pistol — broadcasts OnEquipped, activates aim offset. */
	UFUNCTION(BlueprintCallable, Category="Pistol")
	void Equip();

	/** Holster the pistol — broadcasts OnHolstered, deactivates aim offset. */
	UFUNCTION(BlueprintCallable, Category="Pistol")
	void Holster();

	/**
	 * Scripted gate. When false, StartFire silently does nothing.
	 * Used by the scenario system to lock shooting outside specific scenes.
	 * Default is false — shooting is OFF until the scenario unlocks it.
	 */
	UFUNCTION(BlueprintCallable, Category="Pistol")
	void SetCanFire(bool bAllowed) { bCanFire = bAllowed; }

	// --- Public API: state queries ---

	UFUNCTION(BlueprintPure, Category="Pistol")
	bool IsEquipped() const { return bIsEquipped; }

	UFUNCTION(BlueprintPure, Category="Pistol")
	bool IsReloading() const { return bIsReloading; }

	UFUNCTION(BlueprintPure, Category="Pistol")
	bool CanFire() const { return bCanFire; }

	UFUNCTION(BlueprintPure, Category="Pistol")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	UFUNCTION(BlueprintPure, Category="Pistol")
	int32 GetReserveAmmo() const { return ReserveAmmo; }

	UFUNCTION(BlueprintPure, Category="Pistol")
	int32 GetMagazineCapacity() const { return MagazineCapacity; }

	// --- Delegates: things the rest of the game can react to ---

	/** Broadcast when a round successfully fires. Carries the line-trace HitResult. */
	UPROPERTY(BlueprintAssignable, Category="Pistol|Events")
	FOnPistolFired OnFired;

	/** Broadcast when the trigger is pulled with an empty magazine. */
	UPROPERTY(BlueprintAssignable, Category="Pistol|Events")
	FOnPistolDryFire OnDryFire;

	UPROPERTY(BlueprintAssignable, Category="Pistol|Events")
	FOnReloadStarted OnReloadStarted;

	UPROPERTY(BlueprintAssignable, Category="Pistol|Events")
	FOnReloadFinished OnReloadFinished;

	UPROPERTY(BlueprintAssignable, Category="Pistol|Events")
	FOnPistolEquipped OnEquipped;

	UPROPERTY(BlueprintAssignable, Category="Pistol|Events")
	FOnPistolHolstered OnHolstered;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Editor-tunable parameters ---

	/** How many rounds the magazine holds when full. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pistol|Ammo", meta=(ClampMin="1"))
	int32 MagazineCapacity = 12;

	/** Reserve ammo the player starts with (rounds available for reload). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pistol|Ammo", meta=(ClampMin="0"))
	int32 StartingReserveAmmo = 36;

	/**
	 * Rounds per minute. 360 RPM = 0.167s between shots — feels like a
	 * deliberate semi-auto pull. Real pistols max around 600 RPM.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pistol|Firing", meta=(ClampMin="1"))
	float FireRateRPM = 360.f;

	/** How far the bullet trace reaches, in centimeters. 10000 = 100m. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pistol|Firing", meta=(ClampMin="100"))
	float FireRange = 10000.f;

	/**
	 * Reload duration in seconds. Will be replaced by montage length when
	 * the AnimBP-driven reload is wired up next session; for now it's a
	 * simple timer.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pistol|Firing", meta=(ClampMin="0.1"))
	float ReloadDuration = 1.8f;

	/**
	 * Name of the muzzle socket on the pistol mesh. Bullet traces and
	 * future muzzle FX originate here. Must match the socket name we
	 * created on SK_Pistol_01_LP — currently "Muzzle".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pistol|Firing")
	FName MuzzleSocketName = TEXT("Muzzle");

private:
	// --- Runtime state ---

	int32 CurrentAmmo = 0;
	int32 ReserveAmmo = 0;
	bool bCanFire = false; // scenario gate; unlocked by the story system
	bool bIsEquipped = false;
	bool bIsReloading = false;
	bool bTriggerHeld = false;

	/** World time of the most recent shot, used for fire-rate cooldown. */
	float LastFireTime = -FLT_MAX;


	/** Internal: actually fire a round (assumes all gates have passed). */
	void FireOnce();

	/** Internal: complete a reload after the duration elapses. */
	void FinishReload();

	/** Timer handle for the reload completion. */
	FTimerHandle ReloadTimerHandle;

	/** Convert RPM to seconds-between-shots. */
	float GetFireCooldown() const { return 60.f / FMath::Max(FireRateRPM, 1.f); }
};
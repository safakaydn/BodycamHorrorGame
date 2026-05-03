// Copyright belongs to Real Interactive Studio, 2025


#include "PistolComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

// ---------------------------------------------------------------------------
// Construction & lifecycle
// ---------------------------------------------------------------------------

UPistolComponent::UPistolComponent()
{
	// We don't need per-frame Tick yet — fire/reload run on timers and the
	// scenario gate is event-driven. Leaving Tick disabled saves cost.
	// We'll flip this back on later if we add per-frame work like sway
	// driven by velocity.
	PrimaryComponentTick.bCanEverTick = false;
}

void UPistolComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize ammo state from the editor-tunable defaults.
	CurrentAmmo = MagazineCapacity;
	ReserveAmmo = StartingReserveAmmo;

}

void UPistolComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// Intentionally empty. Reserved for future per-frame work.
}

// ---------------------------------------------------------------------------
// Firing
// ---------------------------------------------------------------------------

void UPistolComponent::StartFire()
{
	bTriggerHeld = true;

	// Gate 1: scenario lock. The story system decides when shooting is allowed.
	if (!bCanFire)
	{
		return;
	}

	// Gate 2: must be equipped and not mid-reload.
	if (!bIsEquipped || bIsReloading)
	{
		return;
	}

	// Gate 3: fire rate cooldown.
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (World->GetTimeSeconds() - LastFireTime < GetFireCooldown())
	{
		return;
	}

	// Gate 4: ammo. Empty mag → dry-fire event, no shot.
	if (CurrentAmmo <= 0)
	{
		OnDryFire.Broadcast();
		return;
	}

	FireOnce();
}

void UPistolComponent::StopFire()
{
	bTriggerHeld = false;
}

void UPistolComponent::FireOnce()
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	CurrentAmmo--;
	LastFireTime = World->GetTimeSeconds();

	// The line trace is intentionally NOT done here yet. It needs the muzzle
	// socket transform from the pistol mesh, which lives on the character —
	// not on this component. We wire that in next session when we plug the
	// component into ABodycamCharacter.
	//
	// For now we broadcast OnFired with an empty HitResult. Listeners (the
	// AnimBP, eventual HUD, etc.) react to "a shot fired" without needing
	// the actual hit data yet.
	FHitResult Hit;
	OnFired.Broadcast(Hit);
}

// ---------------------------------------------------------------------------
// Reload
// ---------------------------------------------------------------------------

void UPistolComponent::Reload()
{
	// No-op cases.
	if (bIsReloading || !bIsEquipped)
	{
		return;
	}
	if (CurrentAmmo >= MagazineCapacity || ReserveAmmo <= 0)
	{
		return;
	}

	bIsReloading = true;
	OnReloadStarted.Broadcast();

	// Schedule the actual ammo transfer for ReloadDuration seconds from now.
	// Next session we'll replace this fixed timer with one driven by the
	// reload montage's actual length (so animation and gameplay stay in sync).
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ReloadTimerHandle,
			this,
			&UPistolComponent::FinishReload,
			ReloadDuration,
			false);
	}
}

void UPistolComponent::FinishReload()
{
	const int32 AmmoNeeded = MagazineCapacity - CurrentAmmo;
	const int32 AmmoToLoad = FMath::Min(AmmoNeeded, ReserveAmmo);

	CurrentAmmo += AmmoToLoad;
	ReserveAmmo -= AmmoToLoad;

	bIsReloading = false;
	OnReloadFinished.Broadcast();
}

// ---------------------------------------------------------------------------
// Equip / Holster
// ---------------------------------------------------------------------------

void UPistolComponent::Equip()
{
	if (bIsEquipped)
	{
		return;
	}

	bIsEquipped = true;
	OnEquipped.Broadcast();
}

void UPistolComponent::Holster()
{
	if (!bIsEquipped)
	{
		return;
	}

	bIsEquipped = false;

	// Cancel an in-flight reload if the player holsters mid-reload.
	if (bIsReloading)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ReloadTimerHandle);
		}
		bIsReloading = false;
	}

	OnHolstered.Broadcast();
}
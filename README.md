# Bodycam Horror Game

A psychological thriller / drama game built in Unreal Engine 5.6.
First-person bodycam-style POV, linear scenario-driven gameplay,
cinematic cutscenes between gameplay segments.

> 🚧 **Status:** In active solo development. Not yet playable. No public builds.

---

## Premise

"The player alternates between two perspectives
— a policeman and a child — across a single tense night..." 

## Key design pillars

- **Scenario first, mechanics second.** The drama is the point; mechanics
  serve the story.
- **Bodycam realism.** Headbob, breathing, jump/landing kicks — every
  visual cue reinforces "this is a real body in a real space."
- **Two playable characters.** A policeman with a pistol; a child with
  only the most basic interactions. Different perspectives on the same
  events.
- **Cinematic transitions.** Animated door interactions double as
  scene/episode transitions, hiding loads and reinforcing cinematic feel.

## Planned mechanics

- First-person movement with sprint and crouch
- Bodycam-style camera shake (headbob, breathing, landing kicks)
- Flashlight with free-aim feel
- Pistol aiming and shooting (Policeman only)
- Inventory: keys and bullets
- Animated door interactions used as cinematic transitions
- Cutscenes between episodes

---

## Tech stack

- **Engine:** Unreal Engine 5.6
- **Language:** C++ for game logic, Blueprints for composition and tuning
- **IDE:** Visual Studio 2022 (build) + VS Code (editing)
- **Version control:** Git + Git LFS, hosted on GitHub
- **Studio:** Real Interactive Studio (solo dev)

---

## Architecture

### Characters
- **`ABodycamCharacter`** — base playable character. First-person camera,
  movement, sprint, FOV interpolation, input wiring.
- *(Planned)* **`APolicemanCharacter`** — extends base with pistol logic.
- *(Planned)* **`AKidCharacter`** — extends base with simpler interactions
  and a different shake feel.

### Components
- **`UBodycamShakeComponent`** — headbob, breathing (Perlin-noise based),
  strafe roll, jump/landing kicks. Animates a target scene component
  passed in via `InitializeForPivot()`.
- **`UInteractionComponent`** — line-trace-based detection of nearby
  interactables. Exposes `bIsFocusingInteractable` and
  `HandTargetLocation` for animation Blueprints.
- *(Planned)* **`UFlashlightComponent`** — currently embedded in
  `ABodycamCharacter`; will be extracted.
- *(Planned)* **`UPistolComponent`** — Policeman-only weapon logic.

### Interfaces
- **`IInteractable`** — implemented by anything the player can interact
  with (doors, items, scripted triggers). Exposes interaction mode
  (`Simple`, `Inspect`, `DoorContext`, `Forced`) and the world location
  the hand should reach toward.

---

## How to build

1. Clone the repo (LFS will pull binary assets):
2. Right-click `BodycamHorrorGame.uproject` → **Generate Visual Studio
   project files**.
3. Open `BodycamHorrorGame.sln` in Visual Studio 2022.
4. Build (`Ctrl+Shift+B`).
5. Open `BodycamHorrorGame.uproject` in Unreal Editor 5.6.

### Required external assets (not in repo)

The following Fab / Marketplace asset packs are referenced but not
committed (license restrictions; recoverable from the original purchase):

- `Content/Fab/` — [BRIEFLY DESCRIBE WHAT'S IN HERE OR LIST THE PACKS]
- `Content/MC_Doors/` — animated door skeleton + animation pack

Without these, some Blueprints will fail to load. Install the same
packs from your own Fab account into the matching folders.

---

## License

All rights reserved. Copyright © 2025 Real Interactive Studio.
This repository is published for development tracking. Code and assets
may not be reused without written permission.

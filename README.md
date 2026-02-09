# DF Game (CLI prototype)

This project provides a CLI-first skeleton for a multiplayer card game inspired by "dark forest" themes.
It runs locally on Windows, macOS, and Linux. Players can host a room or join by entering the host IP.

## Phases (rule scaffolding)

The core loop is organized into three phases, each executed after all players finish actions:

### Preparation

1. Update global projectile positions.
2. Resolve Type I effects and check player survival.
3. Affected players make survival decisions (placeholder hook).
4. Update and resolve Type III effects (energy production, buildings).

### Play

Players choose **one** action (discard+draw or play+draw), can trigger active skills,
and spend energy. Decisions are simultaneous and hidden until resolution.

### Resolution

1. Apply Type II effects immediately (highest priority).
2. Spawn projectiles based on played cards (Type I).
3. Spawn Type III buildings/ongoing effects.

The current code preserves placeholder hooks for these phases and effect types.

## Observability (player information)

- Players can observe a system's **visible state** (star exists, 2D collapse, etc.).
- Player presence and total surviving players are **not observable**.
- Observation follows light-speed propagation (speed = 1).
  - Distance 1: current state.
  - Distance 2: state from 1 round ago.
- When a projectile is within distance 1, a warning is delivered at the **start of the Play phase**
  with the projectile's current position.

The CLI scaffold includes placeholders for observation reports and warnings.

## Card catalog (initial data)

A placeholder catalog mirrors the latest provided card list (broadcast, energy, defense,
strike, special, buildings, and skills). It is stored in `src/Cards.*` for later gameplay logic.
Broadcast cards are stored as separate cooperation/stealth variants, and the current
catalog includes explicit broadcast ranges (1/2/∞). Energy production is applied once
per round at the end of the Preparation phase. Harmony Eye is indestructible by attacks
and awakens the Time Interference ability.

## Broadcast resolution (timing rules)

- The star map is a connected graph; distance is the shortest path length.
- Broadcast ranges: star=1, universe=2, super-distance=∞.
- A broadcast played in the **previous Resolution** is answered and resolved in the
  **next Preparation**.
- The broadcaster learns the number of responses and randomly selects one responder
  for energy settlement.
- The listening base can ignore broadcasts and triggers no response effects.
- Responders are filtered by broadcast range using shortest-path distance.

## Roles (initial design notes)

- Roles are assigned randomly and uniquely at game start, along with a home system node ID.
- If a home system is hit by Time Interference, the player is immediately eliminated (no near-death state).

### Human

- **Interstellar Migration**: when near death, may spend all energy to move to a random system that is not destroyed,
  occupied, or colonized, and mark it as a colony. Usable once per game; Time Interference can reset the usage.

### Observer

- **Observation**: once every two rounds, check if a target system has a player.
- If empty, may build a **Listening Station** that allows broadcast send/receive or strike forewarning.
  Communication ignores distance and is established immediately; next round the link is active.

### Trisolarian

- **Tech Lockdown**: spend 4 energy to discard all building cards in a target system (no survival impact).
  Starts with one use; then gains one use every 5 rounds (no stacking). This effect is a projectile (speed 1).

### Conquer

- **Conquest**: once every 5 rounds, spend energy to launch **Interstellar Expedition** without a card.
  Expedition is a projectile with speed 1.

### Evaluator

- **Evolution**: after destroying a faction or challenging via Interstellar Expedition (success or failure),
  gain that faction's skill.

## Multiplayer CLI usage

```bash
./df_game --host --players 3 --name Host
./df_game --join --ip <host_ip> --name ClientA
./df_game --join --ip <host_ip> --name ClientB
```

Type chat messages and press Enter to broadcast. Use `quit` to exit.

## Fixed asset paths

Assets are expected under `./assets/` (see `assets/README.md`).

## AI model

AI model loading is stubbed via `AiClient` and defaults to `models/ai/default.model`.

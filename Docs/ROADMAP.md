# Roadmap

## Phase 0 — foundation (in progress)
- [x] Establish repository and development branch.
- [x] Record system boundaries and safety invariants.
- [ ] Confirm exact Orbiter build, SDK package, compiler, and architecture.
- [ ] Build the portable core and test harness with the user's toolchain.

## Phase A — career and persistence
- [ ] New company / career state with configurable starting capital.
- [ ] Account ledger with idempotent transactions.
- [ ] Versioned save/load and crash-safe replacement.
- [ ] Deterministic simulation-time advancement.

## Phase B — stations, markets, and cargo
- [ ] Solar-system location identifiers and station definitions.
- [ ] Market inventories and buy/sell transactions.
- [ ] Abstract cargo manifests and mass accounting.
- [ ] Docking/location observation through the Orbiter bridge.

## Phase C — contracts
- [ ] Delivery and transport contracts.
- [ ] Evidence-based eligibility and completion.
- [ ] Idempotent settlement and contract history.

## Phase D — production and infrastructure
- [ ] Production recipes, inputs, outputs, and cycle timing.
- [ ] Station infrastructure and operating costs.
- [ ] Supply/demand effects and market balancing.

## Phase E — fleet and company growth
- [ ] Vessel profiles and operating costs.
- [ ] Stock-vessel adapters, then documented third-party adapters.
- [ ] Company levels, hiring, and fleet management.

## Acceptance tests for the first playable release
- Module loads in the confirmed Orbiter installation.
- Career can be created, saved, closed, and reloaded.
- Purchase changes both account ledger and cargo manifest exactly once.
- Docking is detected from Orbiter state, not a UI button.
- Delivery is rejected for the wrong vessel, location, or cargo.
- Valid delivery pays once, including after save/reload.
- Time acceleration and large simulation-time jumps do not duplicate production or payouts.

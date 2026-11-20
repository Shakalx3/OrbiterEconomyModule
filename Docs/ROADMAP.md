# Roadmap

## Phase 0 — foundation (in progress)
- [x] Establish repository and development branch.
- [x] Record system boundaries and safety invariants.
- [x] Confirm Orbiter 2010 SDK headers are present in the repository.
- [x] Add first LuaMFD telemetry probe and validate it in Orbiter 2010.
- [x] Build the portable core and test harness in GitHub CI.
- [x] Verify native landed-state, vessel-position, surface-base enumeration, and 5 km distance path in a diagnostic MFD.
- [x] Validate `VESSEL::GroundContact()` in a separate diagnostic MFD in the target Orbiter installation.
- [x] Implement and runtime-validate the background bridge for takeoff/landing timestamps, vessel identity, body/base and distance.
- [x] Add portable takeoff/landing monitors and an idempotent simulation event journal with deterministic event IDs.
- [x] Add versioned journal serialization, atomic replacement, and recovery tests for an incomplete final record.
- [x] Build and runtime-validate the bridge-to-event-journal integration.
- [x] Build and runtime-validate journal persistence across scenario reloads.
- [x] Add a native MFD flight log with paging over persisted events.
- [x] Runtime-validate the flight log MFD in Orbiter 2010.
- [x] Add native docking/undocking observation and publish those events from actual dock-port state.
- [ ] Runtime-validate docking/undocking event detection and journal display in Orbiter 2010.
- [ ] Connect verified events to contract/cargo eligibility and settlement.

## Phase A — career and persistence
- [ ] New company / career state with configurable starting capital.
- [ ] Account ledger with idempotent transactions.
- [ ] Versioned save/load and crash-safe replacement.
- [ ] Deterministic simulation-time advancement.

## Phase B — stations, markets, and cargo
- [ ] Solar-system location identifiers and station definitions.
- [ ] Market inventories and buy/sell transactions.
- [x] Add portable abstract cargo manifests with transfer history, mass accounting and idempotent load/unload operations.
- [x] Persist cargo manifest transfer history with versioned, atomic, idempotent recovery.
- [x] Add a portable authorization gate requiring verified landed + ground-contact spaceport state or an explicitly recognized OEM cargo-service dock.
- [x] Require an explicit trusted cargo-service registry; arbitrary docked vessels cannot authorize transfers.
- [x] Add versioned, atomic persistence and corruption-safe loading for the trusted cargo-service allowlist.
- [x] Load the cargo-service allowlist in the native bridge and annotate actual dock/undock events for registered counterpart vessels.
- [ ] Build and runtime-validate cargo-service recognition in Orbiter 2010.
- [ ] Add stock-vessel cargo adapters only where a documented physical cargo interface exists.
- [ ] Docking and spaceport-location observation through the Orbiter bridge.

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
- Spaceport landing requires landed + ground contact + distance <= 5 km from a base on the same body.
- Delivery is rejected for the wrong vessel, location, or cargo.
- Valid delivery pays once, including after save/reload.
- Time acceleration and large simulation-time jumps do not duplicate production or payouts.

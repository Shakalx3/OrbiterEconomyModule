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
- [x] Runtime-validate docking/undocking event detection and journal display in Orbiter 2010.
- [ ] Connect verified events to contract/cargo eligibility and settlement.

## Phase A — career and persistence
- [ ] New company / career state beyond the explicitly configured opening account.
- [x] Company account ledger with idempotent transactions.
- [x] Versioned global economy journal with durable append and transactional recovery.
- [x] Use Orbiter simulation time for cargo and market operation timestamps.

## Phase B — stations, markets, and cargo
- [ ] Solar-system location identifiers and station definitions.
- [x] Portable market catalog, quote engine, and atomic BUY/SELL transaction engine.
- [x] Global journal records market trades in the same ordered stream as cargo exchanges.
- [x] Native Station Market MFD (key M) with one-unit BUY/SELL operations at registered docked services.
- [x] Runtime-validate market purchases, sales, and correct opening balance in Orbiter 2010. (The user confirmed BUY/SELL and opening balance; restart recovery remains a separate check.)
- [x] Add portable abstract cargo manifests with transfer history, mass accounting and idempotent load/unload operations.
- [x] Add portable station-side stock accounting with explicit initial stock, load debits, unload credits, quantity bounds and idempotent transfer IDs.
- [x] Add a unified durable exchange journal that commits station-stock and vessel-manifest projections from one record and recovers them transactionally.
- [x] Persist cargo manifest transfer history through the global versioned, durable, idempotent economy journal.
- [x] Add a portable authorization gate requiring verified landed + ground-contact spaceport state or an explicitly recognized OEM cargo-service dock.
- [x] Require an explicit trusted cargo-service registry; arbitrary docked vessels cannot authorize transfers.
- [x] Add versioned, atomic persistence and corruption-safe loading for the trusted cargo-service allowlist.
- [x] Load the cargo-service allowlist in the native bridge and annotate actual dock/undock events for registered counterpart vessels.
- [x] Build and runtime-validate cargo-service registry loading and recognized-service dock/undock annotations in Orbiter 2010.
- [x] Integrate the global economy journal into native bridge startup and transfer commits; old test journals are ignored without being modified.
- [x] Persist explicit per-service initial-stock configuration and validate it before exchange replay; a missing file means zero stock.
- [x] Runtime-validate native global-journal exchange commit and recovery across scenario restart in Orbiter 2010.
- [x] Specify the global economy journal format, ordered replay model, unlimited station funds, and legacy migration safety.
- [x] Implement and test the portable global economy journal across stations and operation types.
- [x] Add a read-only legacy journal inspector that detects malformed files and refuses multiple non-empty streams without provable chronology.
- [x] Adopt clean-start policy: ignore test-era per-station exchange and per-vessel manifest journals; preserve them untouched and seed the global economy from explicit configuration.
- [x] Connect native cargo exchange commits and recovery to the global economy journal.
- [x] Build the updated Win32 Orbiter 2010 bridge and publish the rolling native release.
- [x] Runtime-validate clean global-journal startup, first exchange commit, and recovery after scenario restart in Orbiter 2010.
- [ ] Add stock-vessel cargo adapters only where a documented physical cargo interface exists.
- [x] Docking and spaceport-location observation through the Orbiter bridge.

## Phase C — contracts
- [x] Portable delivery contract model with vessel, cargo, quantity, origin, destination, reward and deadline.
- [x] Evidence-based eligibility using post-acceptance origin loads and verified destination landing/docking events.
- [x] In-memory idempotent payout through the company ledger.
- [ ] Durable contract acceptance/completion/payout history in the global economy journal.
- [ ] Native bridge integration and contract MFD.
- [ ] Runtime-validate delivery and payout recovery after scenario restart.

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

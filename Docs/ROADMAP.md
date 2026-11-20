## Procedural planetary opening stock (planned)

- [ ] On first registration of a planetary surface base, optionally generate its initial cargo stock from a deterministic, persisted planet-profile seed rather than leaving every new port empty.
- [ ] Derive eligible cargo categories and quantity ranges from the celestial body's properties and a future explicit economy/biome/industry profile; use bounded randomness only within those rules.
- [ ] Persist the generated result in the normal per-location stock config so reloads never reroll it; never regenerate or overwrite an existing stock file or a location with prior transactions.
- [ ] Keep a configurable empty-stock mode and make generated starting stock clearly distinguishable from physically loaded vessel cargo. Do not infer real industrial output from Orbiter properties alone.

# Roadmap

### Phase 0 — first-run installation bootstrap

- [ ] Make the native module self-initializing so installation can consist of copying the single OEM DLL into the Orbiter module folder.
- [ ] On first load, resolve the module's own data root and create every required OEM directory automatically (including `OEMData` and any dedicated subdirectories).
- [ ] Generate a complete, valid starter configuration set for a new installation: account definition, cargo catalogue, empty cargo-service/station registries, and any required versioned state headers. Define safe, documented gameplay defaults rather than requiring the user to author files.
- [ ] Create only missing files. Never overwrite existing user configuration, campaign journals, stock files, or player progress; validate existing files and report malformed/incompatible data clearly.
- [ ] Keep generated planetary/station stock empty unless an explicit future starting-stock profile is enabled; first-run bootstrap must not fabricate cargo or physical vessel inventory.
- [ ] Make all ordinary setup and ongoing administration available in-game. Users must not need to edit configuration files or create folders manually.
- [ ] Add a first-run/bootstrap status report to the OEM MFD/log and tests for missing files, partial installations, existing custom files, and repeat launches.
- [ ] Verify a clean installation by copying only the built DLL into a fresh Orbiter installation, launching the simulator, and confirming that OEM creates its complete data tree and becomes ready without manual file preparation.

# Phase 0 — foundation (in progress)
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
- [x] Generalize the native market location resolver to accept either a registered docked station or a landed vessel within 5 km of an Orbiter surface base.
- [x] Use the same explicit per-location stock configuration and global economy journal for planetary bases and stations.
- [ ] Runtime-validate planetary-base BUY/SELL, stock accounting, and recovery after scenario restart in Orbiter 2010.

## Phase A — career and persistence
- [ ] New company / career state beyond the explicitly configured opening account.
- [x] Company account ledger with idempotent transactions.
- [x] Versioned global economy journal with durable append and transactional recovery.
- [x] Use Orbiter simulation time for cargo and market operation timestamps.

## Phase B — stations, markets, and cargo
- [ ] Solar-system location identifiers and station definitions, shared by stations and planetary bases.
- [x] Add a versioned planetary spaceport registry with stable body/base IDs, coordinates, parent-body mass/radius, derived gravity/escape velocity, and VTOL pad/ILS metadata.
- [ ] Runtime-validate the generated spaceport registry against the active Orbiter installation and compare its entries with the simulator's configured surface bases.
- [x] Portable market catalog, quote engine, and atomic BUY/SELL transaction engine.
- [x] Global journal records market trades in the same ordered stream as cargo exchanges.
- [x] Native Location Market MFD (key M) with one-unit BUY/SELL operations at registered docked stations or verified landed planetary bases.
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

### Live vessel refuelling — planned

- [ ] Use Orbiter's native propellant API to inspect live vessel fuel resources: `GetPropellantCount`, `GetPropellantHandleByIndex`, `GetPropellantMass`, and `GetPropellantMaxMass`.
- [ ] Refuel the actual running vessel through `SetPropellantMass` (or `SetFuelMass` for the default resource); never simulate refuelling by changing only CargoManifest, vessel empty mass, or an OEM-only counter.
- [ ] Offer fuel purchases at registered economic stations and verified planetary spaceports. Docking must be to a registered economic station; surface refuelling requires the existing landed + ground-contact + same-body spaceport verification. Arbitrary nearby or docked vessels must not qualify.
- [ ] Calculate purchasable quantity from the live tank capacity and current mass; reject overfill, invalid handles, non-finite values, negative quantities, and transactions with no available tank capacity.
- [ ] Initially expose propellant resources by stable per-vessel resource slot/handle for selection and display. Orbiter's generic propellant API exposes resource handles and mass/capacity, but does not provide a universal human-readable fuel-type name; do not guess that different resources are chemically interchangeable. Add vessel-specific resource-to-product mappings only through explicit adapters/configuration.
- [ ] Add explicit station/spaceport fuel stock and price configuration. Missing fuel stock means zero available fuel; never create free or implicit starting stock. Keep fuel stock separate from cargo manifests and cargo exchange accounting.
- [ ] Commit each refuelling purchase idempotently to the global economy journal, charging the company account and debiting the location's fuel stock exactly once; persist enough before/after tank state to detect and safely reconcile interrupted operations.
- [ ] Treat the SDK's `SetPropellantMass` as the physical commit operation: validate before mutation, read back the resulting mass, and define rollback/recovery behavior for failed writes or interrupted transactions before enabling real purchases.
- [ ] Add an in-game Refuel MFD/terminal flow showing location, selected tank/resource, current mass, capacity, available stock, unit price, requested quantity, and total cost; support partial and full refuelling.
- [ ] Runtime-test live refuelling at both an orbital station and a planetary spaceport, including multiple propellant resources, full tanks, insufficient stock/funds, save/reload, time acceleration, and duplicate-operation recovery.
## Phase C — contracts
- [x] Portable delivery contract model with vessel, cargo, quantity, origin, destination, reward and deadline.
- [x] Evidence-based eligibility using post-acceptance origin loads and verified destination landing/docking events.
- [x] In-memory idempotent payout through the company ledger.
- [ ] Durable contract acceptance/completion/payout history in the global economy journal, with station and planetary-base destinations treated equally.
- [ ] Native bridge integration and contract MFD.
- [ ] Runtime-validate delivery and payout recovery after scenario restart.

### Mission generation and contract system — planned

- [ ] Expand the existing delivery-contract prototype into a durable mission system, initially offering standalone missions while keeping the data model extensible to future multi-stage chains and campaigns.
- [ ] Generate mission offers procedurally from a curated pool of objective definitions and compatible templates; validate every generated offer against actual registered locations, cargo catalogue, available stock, and supported Orbiter observations before publishing it.
- [ ] Assemble mission flavor text procedurally from reusable, context-aware fragments (issuer, reason, task, instructions, deadline, and optional details); generated prose must never contradict the actual objective or reward.
- [ ] Support two complementary mission classes: player-executed missions verified against actual Orbiter state/events, and abstract missions resolved entirely by the economic simulation without requiring physical NPC vessels.
- [ ] Do not require physical NPC ships, NPC transporters, escorts, rescue targets, or autonomous NPC piloting. Abstract organizations, carriers, customers, suppliers, and competitors may participate as economic agents without being instantiated as Orbiter vessels.
- [ ] Initial player-executed objective pool: cargo delivery, base/station supply, urgent delivery, verified spaceport visit, verified station docking, and research/route visits where the required position and dwell-time evidence can be measured reliably.
- [ ] Keep mission definition, generated mission instance, objective, conditions, reward, penalty, issuer/reputation, and journal/persistence as separate extensible concepts.
- [ ] Persist generator state and generated offers so a restart does not reroll the mission board; persist accepted mission state and all objective evidence/reward settlement idempotently in the global economy journal.
- [ ] On mission failure, apply both a monetary penalty and a reputation reduction. Declining an unaccepted offer has no penalty; cancellation/failure rules for accepted contracts must be explicit per mission.
- [ ] Track reputation independently for each organization/issuer; use it to influence future mission availability and offer quality, without implying the issuer is a physical NPC vessel.
- [ ] Use Orbiter simulation time for deadlines and mission timing; never use wall-clock time.

### Abstract economic agents and background trade — planned

- [ ] Introduce abstract economic agents (organizations, suppliers, customers, carriers, and competitors) that exist only in the OEM economy model and do not require physical Orbiter vessels or NPC navigation AI.
- [ ] Simulate background trade between abstract agents and registered orbital stations/planetary spaceports so the player is not the only economic actor in the solar system.
- [ ] Model each agent's role, home/operating locations, cargo demand/supply, trading preferences, available funds or agreed unlimited-credit policy, and configurable activity cadence.
- [ ] Execute background purchases and sales as real economic transactions: update the relevant location stock, agent inventory/balance where applicable, and market supply/demand using deterministic, auditable transaction records.
- [ ] Never create cargo from nowhere or allow stock to become negative. Every abstract trade must have a valid source, destination, quantity, cargo definition, and sufficient source stock; any production or consumption must be a separate explicitly modelled process.
- [ ] Distinguish abstract economic stock movement from physical vessel cargo and player CargoManifest transfers. Background trade must never imply that a physical ship loaded, flew, docked, or landed.
- [ ] Use simulation time for trade scheduling and resolve elapsed simulation intervals safely after time acceleration or scenario reload, without duplicate transactions or unbounded catch-up bursts.
- [ ] Persist agent definitions/state, background trade history, and scheduling progress with versioned, corruption-safe, idempotent recovery; integrate durable economic effects with the global economy journal.
- [ ] Make background activity configurable, including an option to disable it or tune its intensity, while preserving a coherent economy when disabled.
- [ ] Add observable market activity/history so the player can see that stations and spaceports trade with other economic participants even when no player transaction occurs.

### Player fleet ownership and ship purchasing — planned
- [ ] Ownership default rule: every Orbiter vessel not explicitly registered in `EconomicStationRegistry` is presumed to be player-owned; there are no autonomous physical NPC vessels in the current design.
- [ ] Keep station identity separate from cargo-service permissions: a vessel registered only as a cargo service, but not as an economic station, remains player-owned under the default rule.
- [ ] On scenario load and vessel creation/deletion, derive and reconcile the player fleet from the live Orbiter vessel list plus the station registry; do not require manual registration of ordinary ships.
- [ ] Apply sale/repo-session eligibility only to vessels classified as player-owned by this rule; never sell or repossess registered economic stations.

- [ ] Add a persistent player-owned vessel registry, separate from the general list of vessels currently present in Orbiter; registration/ownership must be explicit and survive scenario reloads.
- [ ] Add an in-game ship catalogue based on vessel classes actually installed in the user's Orbiter configuration (class configuration and required vessel module/assets must exist); do not advertise unavailable vessel classes.
- [ ] Support purchasing an available ship from an eligible station/spaceport, with price, delivery/creation location, and any prerequisites shown before confirmation.
- [ ] Use the Orbiter SDK live vessel-creation API (oapiCreateVessel / oapiCreateVesselEx) to instantiate a real vessel during the running simulation. The SDK documents VESSELSTATUS2 for specifying initial reference body, position/velocity, orientation, landed/free-flight state, and related status.
- [ ] Validate the requested class configuration and spawn state before charging the player; handle creation or durable-transaction failure without charging or leaving an unowned/duplicate vessel. Make purchase IDs idempotent.
- [ ] Choose a safe spawn location and initial state appropriate to the selling location; avoid spawning inside another vessel, inside terrain, or in an invalid orbital state. Planetary surface delivery requires valid base coordinates and a compatible vessel class.
- [ ] Persist ownership independently of vessel handles (which are runtime-only); reconcile registered ownership against vessel names/identities after scenario load and report missing or renamed vessels without silently transferring ownership.
- [ ] Integrate purchases with the company ledger and global economy journal; the vessel must become player property only after the purchase is durably committed.
- [ ] Expose owned fleet, current location/state, acquisition price, and sale eligibility in an in-game fleet management interface.
- [ ] Treat SDK vessel creation as creation of an instance of an already installed vessel class, not arbitrary generation of a new spacecraft design. A genuinely new ship design still requires a vessel class implementation/configuration and its assets.
- [ ] Verify live creation, scenario save/reload persistence, ownership recovery, duplicate prevention, and rollback/failure handling in the target Orbiter runtime before release.
### Debt collection and player asset ownership — planned

- [ ] Allow monetary penalties to exceed the player's available balance; debit available funds and record the remainder as an explicit outstanding debt in the durable economy ledger.
- [ ] Allow the player to repay debt through normal earnings, trading, and further missions; do not automatically prohibit mission acceptance solely because the player is in debt.
- [ ] Add a persistent player-asset registry before implementing repossession. Orbiter does not inherently identify which vessels are player-owned or provide a universal market valuation, so ownership and valuation must be explicit and evidence-based.
- [ ] Model repossession as a probability-based collection action whose likelihood can depend on debt size, age/delinquency, and other documented factors; define and balance the exact probability policy later.
- [ ] Protect the player's sole vessel from repossession. If it is the only registered player-owned vessel, retain the debt and let the player work it off.
- [ ] If the player owns additional eligible assets, sell the minimum necessary asset(s), beginning with the lowest current market value, at the applicable market valuation; apply proceeds to debt and return any surplus to the player's account.
- [ ] If proceeds do not cover the debt, retain the unpaid remainder. Record each seizure, sale, debt payment, and surplus refund as durable, idempotent economy transactions.
- [ ] Define safeguards for valuation, ownership changes, active-vessel protection, and sale integration with actual Orbiter vessel state before enabling automatic repossession.

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

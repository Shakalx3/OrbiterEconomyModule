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

### Live vessel refuelling — in progress

- [ ] Treat all propellant resources on a vessel as one unified OEM fuel commodity, regardless of Orbiter resource type or vessel-specific fuel naming. The player sees one aggregate fuel quantity and one aggregate capacity; no per-engine/per-resource fuel selection or chemical-type distinction is exposed.
- [x] Add a portable, tested aggregate propellant snapshot validator and deterministic multi-resource fill planner (`PropellantPlan`); it rejects invalid values, missing resources, and over-capacity requests without mutating an existing plan.
- [x] Use Orbiter's native propellant API to inspect every live resource: `GetPropellantCount`, `GetPropellantHandleByIndex`, `GetPropellantMass`, and `GetPropellantMaxMass`. Calculate vessel-wide current fuel and maximum capacity by summing the masses/capacities of all valid propellant resources; the read-only OEM Propellant Monitor was runtime-validated against live vessel data and dynamic fuel consumption.
- [ ] Refuel the actual running vessel through `SetPropellantMass` on the underlying resource handles (or `SetFuelMass` where appropriate); distribute the purchased aggregate quantity across available free capacity using a deterministic policy. Never simulate refuelling by changing only CargoManifest, vessel empty mass, or an OEM-only counter.
- [ ] Offer unified-fuel purchases at registered economic stations and verified planetary spaceports. Docking must be to a registered economic station; surface refuelling requires the existing landed + ground-contact + same-body spaceport verification. Arbitrary nearby or docked vessels must not qualify.
- [x] Calculate purchasable quantity from aggregate live capacity minus aggregate current mass; reject overfill, invalid handles, non-finite values, negative quantities, and transactions with no available tank capacity. The portable fill planner is tested; the live read-only monitor validates the Orbiter snapshot path.
- [ ] Treat fuel availability as unlimited at every eligible economic station and planetary spaceport. Do not maintain fuel stock, fuel inventory, or stock debit/credit records; eligibility and the company balance are the only supply-side constraints.
- [ ] Calculate the unified-fuel sale price contextually from both the location type (orbital station versus planetary spaceport, with room for station/port class) and the celestial body on which the location is situated. Use the canonical economic location and its registered body metadata; do not price solely by a station name or the player's vessel.
- [x] Add a portable validated propellant purchase quote model. It carries the canonical location/body context and quoted unit rate, validates the full tank-fill plan and company balance, rounds fractional credit totals upward, and does not depend on fuel stock.
- [ ] Define transparent, configurable pricing inputs and bounded modifiers for location class and celestial-body context (for example, local supply/logistics conditions); keep the resulting quoted unit price stable for the lifetime of a purchase transaction and record the applied price/context in the durable journal. Do not invent real-world fuel chemistry or assume Orbiter's physical body properties alone determine an economically meaningful price.
- [ ] Commit each refuelling purchase idempotently to the global economy journal, charging the company account exactly once and persisting enough before/after per-resource and aggregate tank state to detect and safely reconcile interrupted operations.
- [x] Add a portable prepared-transaction model that captures each resource's before/after mass and capacity alongside the immutable quote; classify restart recovery as finalize-charge, abort-unchanged, rollback-partial, or conflict without guessing.
- [x] Persist immutable PREPARED intent and one-way COMMITTED/ABORTED terminal records in a dedicated append-only, fsync-backed propellant transaction journal, with operation-ID idempotency, transition validation, and corruption/torn-tail refusal. This is separate from the existing v1 economy journal to preserve backward compatibility.
- [x] Add an idempotent propellant-charge record to the existing v1 global economy journal; replay debits the company ledger exactly once, and the portable tests cover duplicate calls, restart recovery, and insufficient-funds refusal.
- [ ] Connect the durable transaction journal to native scenario startup and recover PREPARED operations against re-enumerated Orbiter resources; resource count/order/capacity mismatches must stop for manual resolution.
- [ ] Treat Orbiter propellant mass mutation as the physical commit operation: validate the full distribution plan before mutation, read back the resulting masses, and define rollback/recovery behavior for partial writes or interrupted transactions before enabling real purchases.
- [ ] Add an in-game Refuel MFD/terminal flow showing location, aggregate current fuel, aggregate capacity, unit price, requested quantity, and total cost; support partial and full refuelling without exposing internal resource types.
- [ ] Runtime-test live refuelling at both an orbital station and a planetary spaceport, including vessels with multiple propellant resources, full tanks, insufficient funds, save/reload, time acceleration, partial-write failure handling, and duplicate-operation recovery.

### Delivery-contract runtime coverage — planned

- [x] Runtime-validate delivery, automatic handover, payout, and recovery after scenario restart between orbital stations. The user confirmed the scenario reloads correctly and no duplicate payout occurs.
- [ ] Runtime-validate a delivery contract whose origin is a verified planetary spaceport and destination is a registered orbital station.
- [ ] Runtime-validate a delivery contract whose origin is a registered orbital station and destination is a verified planetary spaceport.
- [ ] Runtime-validate a delivery contract between two planetary spaceports and verify that a same-named base on another celestial body cannot satisfy the destination.
- [ ] For both planetary directions, verify actual landing + ground contact + same-body spaceport proximity, automatic cargo handover, station/base stock accounting as applicable, contract completion, payout, and no duplicate payout after scenario restart.


## Phase C — contracts
- [x] Portable delivery contract model with vessel, cargo, quantity, origin, destination, reward and deadline.
- [x] Evidence-based eligibility using post-acceptance origin loads and verified destination landing/docking events.
- [x] In-memory idempotent payout through the company ledger.
- [x] Durable contract acceptance/completion/payout history in the global economy journal; settlement and company payout are committed as one journal record and replay exactly once.
- [x] Native bridge integration and Delivery Contracts MFD (key D), with offers derived from configured source stock and other registered orbital stations or planetary spaceports; planetary destination inventories are available during journal recovery.
- [x] Runtime-validate delivery and payout recovery after scenario restart in Orbiter 2010. (The user confirmed that the scenario reloads correctly and no duplicate payout occurs.)

### Mission generation and contract system — planned

Current route-quote defaults are centralized in `MissionEconomicsConfig`: nominal estimate speed 10 km/s, 1.5 deadline margin, six-hour base allowance, one credit per estimated kilometre, 100-credit minimum reward, 500,000-credit reward cap, and a one-year maximum allowance. These are economic tuning assumptions, not Orbiter flight-performance predictions.

- [x] Replace first-match delivery destination selection with a complete eligible-location pool and a session-seeded pseudo-random choice, so registry/file order does not dictate every route; keep the selected destination stable while its pending offer is displayed.
- [x] Refresh unaccepted delivery offers once per in-game day; accepted contracts retain their original terms.
- [x] Runtime-test destination distribution across several registered planetary bases and orbital stations, confirming that changing the order of `spaceports.registry` does not change the selection bias. The user confirmed in Orbiter 2010 that destinations are selected randomly with no noticeable directional preference.
- [x] Give each base/station a variable local mission board with zero to three offers, generated from eligible cargo stock and reachable registered cargo-service destinations; an empty board is a valid result.
- [x] Resolve and validate the exact canonical origin location (including celestial body for planetary bases) before generating each mission; persist boards under canonical location keys while retaining legacy stock/journal storage IDs, with v1 board-file migration.
- [x] Use a consistent route-distance model: great-circle surface distance for same-body spaceport routes; Orbiter global endpoint separation as an explicitly typed estimate for station/orbital/inter-body combinations. The latter is a straight-line estimate, never represented as a proven flight path.
- [x] Derive each mission's deadline/time allowance and reward from its route distance through `MissionEconomicsConfig`, with configurable nominal speed, time margin, base allowance, per-kilometre reward, and minimum/maximum bounds; longer routes generally allow more time and pay more.
- [x] Persist generated mission-board contents and per-location generation state in `OEMData/mission_boards.dat`; offers remain stable across MFD refresh and scenario reload, and acceptance removes an offer from the shared board. Accepted missions retain their quoted reward/deadline in the economy journal.
- [ ] Expand the existing delivery-contract prototype into a durable mission system, initially offering standalone missions while keeping the data model extensible to future multi-stage chains and campaigns.
- [x] Generate validated delivery offers procedurally from a curated starter pool of four compatible delivery templates (routine supply, priority delivery, regional logistics, and long-range logistics); validate registered locations, cargo catalogue, available stock, and Orbiter route observations before publishing. The starter templates are delivery variants, not yet separate non-delivery objective mechanics.
- [x] Assemble deterministic mission flavor text from reusable context-aware templates (routine supply, priority shortage, regional logistics, or remote logistics); text is stored with board v3 and carried into the accepted-contract journal, and every briefing matches the actual cargo and destination.
- [x] Add validated objective rules for cargo delivery, verified spaceport visits, and verified station docking; match the exact vessel, target, body, acceptance time, deadline, and required cargo evidence. Visit/docking offers are generated from registered economic locations and presented in a separate MFD.
- [x] Add a portable lifecycle book for non-delivery objectives: acceptance, evidence-based completion, deadline expiry, cancellation, idempotent ledger payout, and replay hooks.
- [x] Add a persistent independent-objective board in `OEMData/objective_mission_boards.dat`, with deterministic zero-to-three visit/docking offers per registered economic origin and daily simulation-time refresh.
- [x] Add OEM Independent Missions MFD (key I), accept/cancel actions, exact Orbiter event verification, event-history reconciliation after restart, automatic payout, and deadline expiry.
- [ ] Runtime-test visit and docking objectives in Orbiter, including payout-once recovery after scenario reload.
- [x] Add durable global-economy-journal records for independent mission acceptance, verified completion evidence, settlement, expiry, and cancellation; replay restores mission state and credits the company ledger exactly once. Portable replay tests pass in CI, and the Win32 Orbiter bridge links successfully with the event-driven objective MFD integration. Runtime testing of the new visit/docking MFD remains pending.
- [x] Automatically load delivery-contract cargo into the vessel's OEM cargo manifest on acceptance, debit origin stock, and commit acceptance plus loading as one durable, replay-safe journal record. This is an accounting manifest, not a physical Orbiter payload API.
- [ ] Support two complementary mission classes: player-executed missions verified against actual Orbiter state/events, and abstract missions resolved entirely by the economic simulation without requiring physical NPC vessels.
- [ ] Do not require physical NPC ships, NPC transporters, escorts, rescue targets, or autonomous NPC piloting. Abstract organizations, carriers, customers, suppliers, and competitors may participate as economic agents without being instantiated as Orbiter vessels.
- [ ] Initial player-executed objective pool: cargo delivery, base/station supply, urgent delivery, verified spaceport visit, verified station docking, and research/route visits where the required position and dwell-time evidence can be measured reliably.
- [ ] Keep mission definition, generated mission instance, objective, conditions, reward, penalty, issuer/reputation, and journal/persistence as separate extensible concepts.
- [x] Persist delivery and independent-objective board offers and per-location generation state separately; daily refresh is based on Orbiter simulation time, while accepted mission state and objective evidence/payouts replay idempotently from the global economy journal.
- [x] On mission failure, apply both a monetary penalty and a reputation reduction. Declining an unaccepted offer has no penalty; cancellation/failure rules for accepted contracts must be explicit per mission. Runtime-validated in Orbiter 2010, including scenario reload.
- [ ] Track reputation independently for each organization/issuer; use it to influence future mission availability and offer quality, without implying the issuer is a physical NPC vessel.
  - [ ] Add stable issuer IDs to delivery and independent mission definitions and persist them in the global journal with backward-compatible replay of legacy records.
  - [ ] Apply expiry penalties to the company account while applying reputation changes only to the issuing organization; keep company-wide reputation separate.
  - [ ] Give generated offers explicit issuer identities, then use issuer reputation to weight mission availability and procedural offer quality without changing already-published offers.
- [ ] Use Orbiter simulation time for deadlines and mission timing; never use wall-clock time.

### Primary resources, starter stock, and background production — in progress

- [x] Add a portable, profile-driven starter-stock initializer that only accepts untouched empty inventories; it never overwrites existing stock or transfer history.
- [x] Add bounded primary-resource source definitions and deterministic production intents using Orbiter simulation time, per-cycle output, and a maximum local stock cap.
- [x] Add versioned, atomically replaced persistence for primary-resource source definitions and scheduling state.
- [x] Add durable primary-production records to the global economy journal; replay adds stock exactly once, and duplicate operation IDs cannot create cargo twice.
- [x] Recover each source's last committed production timestamp from the global journal before scheduling resumes; reject source-definition conflicts and prevent an early extra cycle after a crash between journal commit and config save.
- [x] Create configured starter stock only when a location's stock file is first created; preserve existing empty files and refuse to recreate a missing baseline if the global journal already references that location.
- [ ] Configure initial resource profiles and renewable sources for appropriate planetary bases, orbital stations, and resource regions; keep resource types and rates configurable.
- [x] Load primary-resource source definitions in the native bridge, commit due production using Orbiter simulation time, persist scheduler timestamps, and log each committed production cycle.

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

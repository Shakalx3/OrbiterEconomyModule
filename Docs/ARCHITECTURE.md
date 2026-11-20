# Architecture

## Boundary

Orbiter owns the simulation. OEM observes and records facts exposed by Orbiter; it must not replace orbital mechanics, alter vessel state to satisfy a contract, or infer a successful delivery from a user-interface action.

Orbiter -> Orbiter Bridge (SDK-dependent DLL) -> OEM Core (portable C++) -> MFD / station terminal UI.

The bridge observes simulation time, vessel identity, takeoff, landing, docking and location, then publishes verified events to the core. The core owns accounts, markets, cargo manifests, contracts, company progression and persistence.

## Initial module boundaries

- Core: identifiers, domain types, validation, deterministic rules.
- Simulation event journal: append-only event collection with deterministic event IDs, duplicate/conflict checks, and a versioned disk snapshot written through a temporary file and atomic replacement.
- Ledger: integer-credit accounting, immutable transaction records, idempotency keys.
- Market: station inventories, buy/sell quotes, stock changes.
- Cargo: manifests and ownership/quantity tracking. The portable `CargoManifest` records item IDs, quantities, unit mass, original loading location, vessel identity and transfer history. Load/unload operations require explicit transfer records with stable IDs and are idempotent. This is an OEM accounting manifest, not a claim that Orbiter's stock vessel model exposes a physical cargo inventory. The `CargoServiceRegistry` maintains a versioned, atomically persisted allowlist of exact vessel names; dock authorization requires both actual docking and membership in this allowlist.
- Cargo transfer authorization: `CargoTransferGate` accepts a transfer only when the bridge proves the exact named spaceport with landed + ground contact + same-body base proximity <= 5 km, or when the actual docked counterpart is present in the explicit `CargoServiceRegistry`. A docking flag or vessel name alone is not sufficient. Arbitrary docking, taxi contact, landed flag alone, unknown position, a different base, and distances beyond 5 km are rejected.
- Physical cargo integration: Orbiter stock vessels do not provide one universal cargo-hold inventory API. Vessel mass changes and docking alone are not cargo evidence. Physical cargo systems require documented adapters; otherwise OEM-managed abstract cargo remains authoritative.
- Contracts: acceptance, eligibility checks, evidence-based completion, settlement.
- Company/Fleet: progression, owned vessels, operating costs.
- Persistence: versioned save format, atomic writes, recovery from interrupted writes.
- Orbiter Bridge: SDK-specific callbacks and vessel/docking observation.
- UI: MFD and station terminal. UI commands request actions; they do not assert simulation facts.

## Event and transaction rules

1. Simulation time comes from Orbiter. The economy advances from observed simulation time, not wall-clock time.
2. A delivery requires evidence from the bridge: correct vessel, required cargo, destination, and a verified docking or other explicitly supported handover condition.
3. A contract payout has a stable settlement key. Reprocessing the same event cannot pay twice.
4. Financial changes are recorded as ledger entries. Balances are derived or reconciled against the ledger; silent balance edits are prohibited.
5. Large time jumps and time acceleration are processed deterministically in bounded economic ticks.
6. Saves are versioned and written transactionally. A partially written save must not replace the last valid save.
7. A spaceport landing is valid only when Orbiter reports the vessel landed, the vessel has actual ground contact, and its surface position is within 5,000 m of at least one surface base on the same reference body. The nearest VTOL pad/runway is intentionally irrelevant.
8. Event IDs include the simulation-session key, event type, vessel ID and simulation timestamp in milliseconds. Duplicate publication is ignored; reuse of an ID for different facts is rejected.

## Spaceport landing and flight event detection

The Orbiter 2010 SDK provides two independent signals: `GetFlightStatus() & 1` for Orbiter's landed/inactive state, and `VESSEL::GroundContact() const` for actual contact of at least one touchdown reference point with the planetary surface. The user has validated the second signal in the target simulator: it changes between flight, surface movement/contact, and the settled landed state. Production acceptance requires both signals.

The native bridge is a background `oapi::Module`, not an MFD redraw callback. Its `clbkPostStep()` runs after each physical simulation step, enumerates vessels, and samples both signals. It only enumerates surface bases once both landed and ground-contact are true. It obtains the surface reference with `VESSEL::GetSurfaceRef()`, vessel coordinates with `VESSEL::GetEquPos()`, and bases on that same body with `oapiGetBaseCount()`, `oapiGetBaseByIndex()`, and `oapiGetBaseEquPos()`. Distances use the body's mean radius and the core's great-circle helper; the 5,000 m threshold is inclusive.

The portable `SpaceportLandingMonitor` consumes these observations and emits one arrival event after an observed loss of ground contact followed by a landed + ground-contact state inside the 5 km zone. Ground contact without the landed flag is a transitional surface/taxi state, not an airborne state. This prevents a vessel that was already taxiing when monitoring starts from being treated as a fresh arrival. A vessel already parked also establishes a baseline without an event. A completed touchdown outside the zone disarms the monitor until the next loss of contact.

The portable `FlightTransitionMonitor` emits a takeoff after a grounded baseline followed by actual loss of `GroundContact()`. It preserves the last known surface body/base/distance as departure context. The first observation is always a baseline, so a vessel already airborne when monitoring starts does not generate a fabricated takeoff.

Both event types are published into the core's append-only `SimulationEventQueue`. The bridge persists the queue to `OEMData/events.journal` beside the module DLL after each new event. The journal has a versioned text format, hex-encoded identifiers, and atomic temporary-file replacement; a torn final record is ignored during recovery. The bridge reloads the journal at simulation start and logs the number of restored records. Session IDs include the scenario start MJD plus process/tick identity so repeating a scenario does not reuse earlier event IDs. The journal is not yet exposed to an MFD or connected to contract settlement. Dock/undock event types are reserved in the domain model but their native observation adapter is a later increment.

## Compatibility strategy

OEM should support stock Orbiter vessels first through a vessel identity/profile adapter. Third-party cargo systems (including UACS and Cargo Deck MFD) are investigated as integration targets; no code is copied and no compatibility is promised until APIs and licenses are reviewed.

The initial market map covers Earth orbit, the Moon, and Mars in greater depth. The data model is solar-system-wide so further locations can be added without changing core rules.

## Current implementation boundary

The portable core includes the ledger, surface-distance calculation, tested landing and takeoff transition monitors, an idempotent event queue, and a versioned event-journal store with atomic replacement and torn-tail recovery tests. The background bridge has been validated in the target Orbiter installation for real takeoff/landing timestamps, vessel identity, body/base identification and distance. Disk persistence is implemented but still requires a fresh native build and runtime reload test. Native docking observation is implemented and has been runtime-validated by the user. The portable abstract cargo manifest, per-vessel versioned transfer-history persistence and transfer authorization gate are implemented and covered by portable tests. Native bridge loading of the trusted cargo-service registry, cargo manifest persistence, and transfer UI are not yet connected; contracts, settlement and economic UI remain outstanding.

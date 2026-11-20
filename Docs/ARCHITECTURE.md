# Architecture

## Boundary

Orbiter owns the simulation. OEM observes and records facts exposed by Orbiter; it must not replace orbital mechanics, alter vessel state to satisfy a contract, or infer a successful delivery from a user-interface action.

Orbiter -> Orbiter Bridge (SDK-dependent DLL) -> OEM Core (portable C++) -> MFD / station terminal UI.

The bridge observes simulation time, vessel identity, docking and location, then exposes verified events to the core. The core owns accounts, markets, cargo manifests, contracts, company progression and persistence.

## Initial module boundaries

- Core: identifiers, domain types, validation, deterministic rules.
- Ledger: integer-credit accounting, immutable transaction records, idempotency keys.
- Market: station inventories, buy/sell quotes, stock changes.
- Cargo: manifests and ownership/quantity tracking. Initial cargo is abstract; physical cargo and third-party vessel adapters are later extensions.
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

## Spaceport landing detection

The Orbiter 2010 SDK provides two independent signals: `GetFlightStatus() & 1` for Orbiter's landed/inactive state, and `VESSEL::GroundContact() const` for actual contact of at least one touchdown reference point with the planetary surface. The user has validated the second signal in the target simulator: it changes between flight, surface movement/contact, and the settled landed state. Production acceptance requires both signals.

The native bridge is a background `oapi::Module`, not an MFD redraw callback. Its `clbkPostStep()` runs after each physical simulation step, enumerates vessels, and samples both signals. It only enumerates surface bases once both landed and ground-contact are true. It obtains the surface reference with `VESSEL::GetSurfaceRef()`, vessel coordinates with `VESSEL::GetEquPos()`, and bases on that same body with `oapiGetBaseCount()`, `oapiGetBaseByIndex()`, and `oapiGetBaseEquPos()`. Distances use the body's mean radius and the core's great-circle helper; the 5,000 m threshold is inclusive.

The portable `SpaceportLandingMonitor` consumes these observations and emits one arrival event after an observed loss of ground contact followed by a landed + ground-contact state inside the 5 km zone. Ground contact without the landed flag is a transitional surface/taxi state, not an airborne state. This prevents a vessel that was already taxiing when monitoring starts from being treated as a fresh arrival. A vessel already parked also establishes a baseline without an event. A completed touchdown outside the zone disarms the monitor until the next loss of contact.

The bridge currently writes verified events to the Orbiter log. It is an observation/event-generation layer, not yet wired to contract settlement, cargo handover, or the economic UI. The existing `OEMSpaceportTest` and `OEMGroundContactTest` MFDs remain separate diagnostics and are not modified by the bridge.

## Compatibility strategy

OEM should support stock Orbiter vessels first through a vessel identity/profile adapter. Third-party cargo systems (including UACS and Cargo Deck MFD) are investigated as integration targets; no code is copied and no compatibility is promised until APIs and licenses are reviewed.

The initial market map covers Earth orbit, the Moon, and Mars in greater depth. The data model is solar-system-wide so further locations can be added without changing core rules.

## Current implementation boundary

The portable core includes the ledger, surface-distance calculation, and a tested landing-transition monitor. Native diagnostics have verified Launchpad discovery, vessel state, surface-base enumeration, and the 5 km calculation. A background native module now samples all vessels after simulation steps and logs verified landing events. End-to-end in-simulator validation of that background module and connection to the economic contract system remain outstanding.

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
7. A spaceport landing is valid only when Orbiter reports the vessel landed, the vessel has actual ground contact, and its surface position is within 5,000 m of at least one surface base on the same reference body. The nearest VTOL pad/runway is intentionally irrelevant to this rule.

## Spaceport landing detection

The Orbiter 2010 SDK provides a direct ground-contact query: `VESSEL::GroundContact() const`. Its documented result is true when at least one of the vessel's touchdown reference points is in contact with a planetary surface. This is distinct from `GetFlightStatus() & 1`, which reports the vessel's inactive/landed state. Production acceptance requires both signals; neither is substituted for the other.

The native bridge obtains the surface reference with `VESSEL::GetSurfaceRef()`, vessel coordinates with `VESSEL::GetEquPos()`, and enumerates bases on that same body with `oapiGetBaseCount()`, `oapiGetBaseByIndex()`, and `oapiGetBaseEquPos()`. It compares coordinates using the body's mean radius and the core's great-circle distance helper. The 5,000 m threshold is inclusive.

The portable `SpaceportLandingMonitor` consumes bridge observations and emits a single arrival event only on a known transition into a fully verified landing. A vessel already parked when monitoring starts establishes a baseline and does not generate a false arrival. Unknown location evidence is not treated as acceptance or rejection; a completed search with no matching base is a known negative result. The existing `OEMSpaceportTest` MFD remains a diagnostic reference and is not the production event source.

The LuaMFD probe remains telemetry only: its API does not expose base enumeration. The native ground-contact method has now been identified in the SDK header; wiring it into a separate production bridge and validating it in Orbiter remain outstanding.

## Compatibility strategy

OEM should support stock Orbiter vessels first through a vessel identity/profile adapter. Third-party cargo systems (including UACS and Cargo Deck MFD) are investigated as integration targets; no code is copied and no compatibility is promised until APIs and licenses are reviewed.

The initial market map covers Earth orbit, the Moon, and Mars in greater depth. The data model is solar-system-wide so further locations can be added without changing core rules.

## Current implementation boundary

The portable core includes the ledger, surface-distance calculation, and a tested landing-transition monitor. A separate native diagnostic has already verified Launchpad discovery, native vessel access, surface-base enumeration, and the 5 km calculation. The diagnostic has not yet exercised `GroundContact()`; the production Orbiter bridge and end-to-end event delivery are still outstanding.

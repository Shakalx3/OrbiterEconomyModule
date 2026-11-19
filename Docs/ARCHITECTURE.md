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

## Compatibility strategy

OEM should support stock Orbiter vessels first through a vessel identity/profile adapter. Third-party cargo systems (including UACS and Cargo Deck MFD) are investigated as integration targets; no code is copied and no compatibility is promised until APIs and licenses are reviewed.

The initial market map covers Earth orbit, the Moon, and Mars in greater depth. The data model is solar-system-wide so further locations can be added without changing core rules.

## Current implementation boundary

The first code slice is deliberately SDK-independent. No Orbiter SDK header, graphics API, or vessel callback is included until the exact Orbiter build, SDK package, compiler, and target architecture are confirmed.

# Cargo service inventory

The portable `CargoServiceInventory` models stock owned by one exact service-vessel ID. Initial stock is explicitly configured by OEM data; it is never inferred from Orbiter vessel class, mass, docking state, or an imagined station warehouse. Initial stock can be set only before the first transfer. A vessel LOAD consumes service stock; an UNLOAD adds stock. Quantities cannot go below zero, unit mass must remain consistent, and transfer IDs are idempotent with conflicting replays rejected.

This milestone implements the in-memory stock-accounting model and its tests. It is not yet loaded by the native bridge and does not enable transfer controls in Orbiter.

Before UI activation, a crash-safe exchange coordinator must make station inventory and vessel manifest one recoverable transaction. Two independent journal replacements are not sufficient: a crash between them could debit one side without crediting the other. The next persistence step will therefore use a single durable exchange record as the commit point, then recover both projections from it.


## Explicit stock configuration

CargoServiceInventoryConfigStore reads and writes one versioned file per service vessel:

OEM_CARGO_STOCK|1|<hex service vessel ID>
S|<hex cargo ID>|<quantity>|<unit mass kg>

For example, ISS is hex 495353. A deliberately configured example with 12 oxygen canisters at 50 kg each would contain:

OEM_CARGO_STOCK|1|495353
S|6F787967656E5F63616E6973746572|12|50

No default cargo quantities are shipped. A missing service stock file means an empty inventory, not an assumed warehouse. Configuration is rejected as a whole if the header, service ID, duplicate cargo IDs, quantities, or masses are invalid.

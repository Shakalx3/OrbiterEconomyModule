# Cargo service inventory

The portable `CargoServiceInventory` models stock owned by one exact service-vessel ID. Initial stock is explicitly configured by OEM data; it is never inferred from Orbiter vessel class, mass, docking state, or an imagined station warehouse. Initial stock can be set only before the first transfer. A vessel LOAD consumes service stock; an UNLOAD adds stock. Quantities cannot go below zero, unit mass must remain consistent, and transfer IDs are idempotent with conflicting replays rejected.

This milestone implements the in-memory stock-accounting model and its tests. It is not yet loaded by the native bridge and does not enable transfer controls in Orbiter.

Before UI activation, a crash-safe exchange coordinator must make station inventory and vessel manifest one recoverable transaction. Two independent journal replacements are not sufficient: a crash between them could debit one side without crediting the other. The next persistence step will therefore use a single durable exchange record as the commit point, then recover both projections from it.

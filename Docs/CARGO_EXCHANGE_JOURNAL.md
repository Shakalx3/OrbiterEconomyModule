# Unified cargo exchange journal

The portable exchange coordinator treats one durable append-only record as the commit point for both sides of a transfer. A committed record contains the transfer ID, direction, service-vessel ID, client-vessel ID, cargo ID, quantity, unit mass, and simulation time.

For a LOAD, replay debits the service inventory and credits the vessel manifest. For an UNLOAD, replay debits the vessel manifest and credits the service inventory. Both projections are updated in memory only after the single journal append has been flushed to durable storage (FlushFileBuffers on Windows, fsync on POSIX). It never writes two independent journals for one exchange.

The journal header is OEM_CARGO_EXCHANGE|1|<hex service vessel ID>. Complete records are versioned by the header and hex-encode identifiers. Recovery starts from an explicitly configured initial service stock, replays records into candidate copies of the inventory and vessel-manifest map, and publishes both candidates only if the entire complete journal validates. A torn final fragment is discarded; a malformed complete record rejects the replay without partially mutating the supplied projections.

The caller must supply the explicit station-stock seed before recovery. The journal records exchanges, not initial stock configuration. The current implementation is portable core only: the native Orbiter bridge still uses its older per-vessel manifest persistence and does not expose cargo transfer controls. Do not enable transfer UI until native startup/recovery and all transfer commits have been migrated to this coordinator, including a deliberate migration policy for existing per-vessel manifest journals.


## Native bridge integration

The native bridge now loads one stock configuration and one exchange journal per registered service vessel. The stock file is OEMData/cargo_stock_<hex service vessel ID>.cfg; the durable exchange journal is OEMData/cargo_exchanges/exchange_<hex service vessel ID>.journal. At scenario startup, registered services are recovered and their transfer histories are merged into vessel-manifest projections.

Before using the recovered projection, the bridge checks legacy per-vessel manifest journals for vessels represented in the exchange history. In addition, immediately before any transfer, it compares the focused vessel's complete history for that service against the service exchange journal in both directions. This second check also catches legacy journals for vessels that have no exchange records yet; a mismatch blocks the transfer with an explicit migration error. Legacy data is not silently overwritten or discarded. Existing legacy histories therefore require an explicit migration before further transfers can be accepted.

The bridge currently supports station-side stock only for registered cargo-service vessels. The authorization gate can recognize a landed spaceport, but a separate spaceport stock registry has not been implemented; those transfers are explicitly rejected. The Cargo Manifest MFD remains read-only. A separate OEM Cargo Exchange Test MFD exposes one-unit LOAD and UNLOAD commands for controlled validation; it changes only OEM's managed accounting and never changes Orbiter vessel mass or physical cargo.


## Controlled native exchange test

For a deliberately synthetic, ledger-only test, create `OEMData/cargo_stock_495353.cfg` beside the bridge DLL with:

```
OEM_CARGO_STOCK|1|495353
S|54455354|2|1.0
```

`495353` is the UTF-8 hex ID for the registered service vessel `ISS`; `54455354` is the test cargo ID `TEST`. This explicitly seeds two units at 1 kg per unit. It is not a physical Orbiter payload.

In Orbiter, focus a vessel docked to ISS and open the `OEM Cargo Exchange Test` MFD (key E). Press LOAD once: station stock should decrease to one and the vessel OEM manifest should show one TEST unit. Press UNLD once: station stock should return to two and the vessel manifest should return to empty. Restart the scenario and verify the exchange recovery log reports two transfers for ISS; the MFD should still show stock quantity two and an empty vessel manifest. The journal stores both operations durably. Do not use this test MFD as a physical cargo-loading mechanism.

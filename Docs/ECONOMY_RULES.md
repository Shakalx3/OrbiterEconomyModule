# Economy rules — initial design

These are design constraints, not final balance values.

- Currency is represented as integer credits (no floating-point money).
- A market quote is a snapshot with a timestamp and station identifier.
- A purchase is one atomic operation: validate funds and stock, debit the buyer, credit the seller/market account, decrement stock, and add cargo.
- A sale is the inverse and must validate cargo ownership and quantity.
- Every economic operation has a unique operation ID. Replaying it returns the prior result instead of applying it twice.
- Contract eligibility is evaluated from recorded simulation evidence, not from the requested UI action.
- Economy ticks use Orbiter simulation time. Paused simulation does not accrue production or time-based fees.
- Offline play is the default. No network service is required.

Starting capital, prices, margins, and progression thresholds remain configurable and will be balanced only after the first market loop is playable.

## Market catalog and quotes

The portable `CargoCatalog` stores explicit cargo definitions: stable cargo ID, display name, unit mass, station buy price and station sell price. Prices are integer credits; unit mass must be positive and finite; IDs are unique. No catalog entries or prices are invented as runtime defaults.

`MarketQuoteEngine` creates a timestamped quote snapshot for one exact cargo-service vessel. A BUY quote means the player buys from the station and therefore checks current station stock. A SELL quote means the player sells to the station and checks that accepting the quantity cannot overflow station stock. The quote captures station ID, cargo ID, side, quantity, unit price, total integer-credit value, observed station stock and simulation time. It does not reserve stock, move cargo, or transfer money. A later transaction must revalidate the quote against current state and commit account, station inventory and vessel manifest changes through one durable operation.

## Atomic market operation prototype

The portable `MarketTradeEngine` now applies a quoted BUY or SELL against candidate copies of the account ledger, station inventory and vessel manifest. BUY debits the company account, credits the station account, consumes station stock and adds to the vessel OEM manifest. SELL performs the inverse and requires the manifest to contain the requested cargo. All candidate projections are published only if every validation succeeds. The operation ID is shared by the ledger and cargo transfer records, so an exact replay is idempotent and conflicting reuse is rejected.

This is currently an in-memory core transaction prototype. It is covered by portable tests but is not yet crash-durable and is not exposed in the Orbiter MFD. A unified durable market-trade journal and recovery path must be completed before enabling player-facing purchase/sale controls.

## Durable market trade journal

`MarketTradeJournalStore` persists a BUY/SELL as one append-only commit record. The record binds the operation to an exact station vessel ID and client vessel ID, and stores account IDs, cargo ID, side, quantity, unit/total integer-credit price and simulation time. The journal is shared per station and records the exact client vessel on every trade, so station stock and account balances remain one coherent history across multiple ships. Recovery rebuilds a map of all vessel manifests touched by that station's trades. Append is flushed with `FlushFileBuffers` on Windows or `fsync` on POSIX before candidate ledger, station inventory and vessel manifest projections are published in memory. A failed append leaves all three live projections unchanged.

Recovery starts from explicitly configured opening account balances, explicit station stock and an empty manifest for the journal's exact vessel. Records replay into candidate copies; all three projections are published only if every complete record validates. An incomplete final line is discarded. A malformed complete line, wrong station/vessel identity, unavailable account, insufficient funds/stock or catalog mismatch rejects recovery without partially publishing state. The journal does not yet configure account opening balances or catalog data itself; those remain separate explicit inputs.

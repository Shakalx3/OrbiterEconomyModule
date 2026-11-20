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

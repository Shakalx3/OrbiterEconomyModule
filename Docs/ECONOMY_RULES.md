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

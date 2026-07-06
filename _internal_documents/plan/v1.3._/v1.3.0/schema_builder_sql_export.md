# Schema Builder — SQL export panel

**File:** `business_suite/middle_wear/database_management/index.html`  
**Date:** May 9 2026  
**Status:** complete

## What was added

- SQL export panel below the schema builder table editor
- Dialect toggle buttons: PostgreSQL (default active), MySQL, SQLite, MSSQL
- Live syntax-highlighted `<pre>` output regenerated on every dialect switch or schema change
- Type mapping per dialect (e.g. `TEXT` → `LONGTEXT` in MySQL, `NVARCHAR(MAX)` in MSSQL)
- Copy to clipboard button
- Download as `.sql` file button (filename derived from table name + dialect)

## Bug fixed

Temporal dead zone: `let sqlDialect = 'postgres'` was declared after the init block called `sqlRender()`, which read `sqlDialect` before it existed. Fixed by moving the declaration to the top of the init block before `sbRenderTableList()` and `sqlRender()`.

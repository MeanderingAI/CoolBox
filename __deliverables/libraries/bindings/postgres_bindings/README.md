# CoolBox Postgres Bindings

SQL extension packaging surface for PostgreSQL integration.

## Build Behavior

The extension builder validates SQL scripts in this folder.
If `POSTGRES_DSN` and `psql` are available, scripts can also be validated against a live PostgreSQL instance.

## Layout

- `sql/coolbox_extension.sql` - install script
- `sql/uninstall.sql` - teardown script

## Optional live validation

```bash
set POSTGRES_DSN=postgresql://user:pass@localhost:5432/postgres
python _scripts/build_scripts/build_extensions.py postgres_bindings
```

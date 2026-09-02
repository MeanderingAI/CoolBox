# Postgres Extension

## Location

- `_deliverables/libraries/bindings/postgres_bindings`

## Surface

- SQL install script: `sql/coolbox_extension.sql`
- SQL teardown script: `sql/uninstall.sql`

## Setup And Build

Static validation via extension builder:

```bash
python _scripts/build_scripts/build_extensions.py postgres_bindings
```

Optional live validation using PostgreSQL CLI and DSN:

```bash
set POSTGRES_DSN=postgresql://user:pass@localhost:5432/postgres
python _scripts/build_scripts/build_extensions.py postgres_bindings
```

## Packaging Notes

- Release page: https://github.com/MeanderingAI/CoolBox/releases
- Release workflows can publish `postgres-extension-*` artifacts.

-- CoolBox Postgres extension bootstrap script
-- Safe to run multiple times.

CREATE SCHEMA IF NOT EXISTS coolbox;

CREATE TABLE IF NOT EXISTS coolbox.binding_health (
    id BIGSERIAL PRIMARY KEY,
    status TEXT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

INSERT INTO coolbox.binding_health(status)
VALUES ('ok');

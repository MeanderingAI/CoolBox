# email_smtp_server README

Path: `_deliverables/libraries/groups/trekker/IO/EMAIL/smtp_server`

This shared library owns SMTP-style sender registration and outbound send orchestration for the email stack.

## Purpose

- Maintain registered sender accounts
- Normalize send requests
- Forward outbound messages into the mailbox layer

## Depends On

- `email_mailbox`

## Exports

- SMTP account registration helpers
- SMTP unregistration helpers
- Send orchestration entry points

## Build Context

This library is included through the `trekker/IO/EMAIL` package root and linked by `email_center_api`.
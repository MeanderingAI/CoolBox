# email_mailbox README

Path: `_deliverables/libraries/groups/trekker/IO/EMAIL/mailbox`

This shared library holds the mailbox and message persistence model used by the email stack.

## Purpose

- Store mailbox state
- Store message payloads and metadata
- Track read/unread status

## Depends On

- No higher-level EMAIL library dependencies

## Exports

- Mailbox storage helpers
- Message append/read helpers
- Message state query helpers

## Build Context

This library is included through the `trekker/IO/EMAIL` package root and linked by both the SMTP and IMAP layers.
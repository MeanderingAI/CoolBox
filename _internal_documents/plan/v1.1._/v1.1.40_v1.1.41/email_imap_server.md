# email_imap_server README

Path: `_deliverables/libraries/groups/trekker/IO/EMAIL/imap_server`

This shared library provides mailbox retrieval and message read-state operations for the email stack.

## Purpose

- Fetch all messages for a mailbox
- Fetch a single message by identifier
- Mark messages as read

## Depends On

- `email_mailbox`

## Exports

- Mailbox lookup helpers
- Message retrieval helpers
- Read-state update helpers

## Build Context

This library is included through the `trekker/IO/EMAIL` package root and linked by `email_center_api`.
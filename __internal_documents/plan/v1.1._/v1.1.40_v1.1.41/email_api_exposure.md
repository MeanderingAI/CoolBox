# email_api_exposure README

Path: `_deliverables/libraries/groups/trekker/IO/EMAIL/api_exposure`

This shared library describes the external exposure modes for the email API layer.

## Purpose

- Represent REST exposure state
- Represent any optional websocket or event exposure metadata
- Provide a stable summary object for the backend status endpoints

## Depends On

- No direct runtime dependency on the mailbox or SMTP layers

## Exports

- Exposure mode types
- Status summary helpers

## Build Context

This library is included through the `trekker/IO/EMAIL` package root and linked by `email_center_api`.
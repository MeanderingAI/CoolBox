# email_center_api README

Path: `_deliverables/apps/email_center_api`

This backend service powers the email center browser interface and reuses the EMAIL libraries under trekker/IO.

## Related Interface

- Frontend: `_interfaces/email_center`
- Frontend entry logic: `_interfaces/email_center/src/main.js`

## What It Does

- Registers and unregisters SMTP-style sender accounts
- Creates and deletes mailboxes
- Lists and reads mailbox messages
- Sends outbound email into the selected mailbox stream
- Exposes status and exposure mode information

## Main Routes

- `GET /api/email/health`
- `GET /api/email/status`
- `POST /api/email/smtp/register`
- `POST /api/email/smtp/unregister`
- `POST /api/email/mailbox/create`
- `POST /api/email/mailbox/delete`
- `GET /api/email/messages?mailbox=<name>`
- `POST /api/email/messages/read`
- `POST /api/email/send`
- `POST /api/email/exposure`

## Reusable Libraries

- `email_smtp_server`
- `email_imap_server`
- `email_mailbox`
- `email_api_exposure`

## Build And Run

```bash
cd /Users/solid/wkspace/CoolBox
cmake -S . -B build -DBUILD_PRODUCTS=ON
cmake --build build --target email_center_api
./build/_deliverables/apps/email_center_api/email_center_api --port 8022
```
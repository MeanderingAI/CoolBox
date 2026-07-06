# Email Center (Vite + C++ API)

Path: _interfaces/email_center

Browser frontend for email_center_api.

## What It Does

- Register/unregister SMTP-style sender account
- Compose and send email into a selected mailbox
- Show mailbox messages list
- Show live service status from the backend

## Backend Endpoints Used

- GET /api/email/status
- POST /api/email/smtp/register
- POST /api/email/smtp/unregister
- POST /api/email/mailbox/create
- POST /api/email/send
- GET /api/email/messages?mailbox=<name>

## Dev

```bash
cd _interfaces/email_center
npm install
npm run dev
```

Default dev server:
- Vite: 5175

Proxy target:
- /api -> http://localhost:8022 (email_center_api)

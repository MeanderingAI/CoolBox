# email_center README

Path: `_interfaces/email_center`

This browser UI displays mailbox messages and sends email through `email_center_api`.

## What It Does

- Registers and unregisters SMTP-style sender accounts
- Creates a mailbox on demand when sending
- Lets the user compose and send a message
- Lists mailbox messages from the backend
- Shows live backend status in the UI

## Backend Dependency

- API base: `http://localhost:8022`
- Proxy path: `/api`

## Main Views

- Account controls
- Compose and send panel
- Mailbox message list
- Service status panel

## Development

```bash
cd _interfaces/email_center
npm install
npm run dev
```

Default dev server port:

- Vite: `5175`
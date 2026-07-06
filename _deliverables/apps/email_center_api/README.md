# email_center_api

Path: _deliverables/apps/email_center_api

A C++ email control-plane service modeled after call_center_api, backed by reusable EMAIL libraries hosted in trekker/IO and referenced from audio_visual_group.

## Reusable EMAIL Libraries

Path root: _deliverables/libraries/groups/trekker/IO/EMAIL

- email_smtp_server: SMTP-style account registration and outbound send orchestration.
- email_imap_server: mailbox retrieval and read-state operations.
- email_mailbox: mailbox/message persistence model.
- email_api_exposure: REST/WebSocket exposure mode descriptor.

## API Endpoints

- GET /api/email/health
- GET /api/email/status
- POST /api/email/smtp/register
- POST /api/email/smtp/unregister
- POST /api/email/mailbox/create
- POST /api/email/mailbox/delete
- GET /api/email/messages?mailbox=<name>
- POST /api/email/messages/read
- POST /api/email/send
- POST /api/email/exposure

## Build and Run

```bash
cd /Users/solid/wkspace/CoolBox
cmake -S . -B build -DBUILD_PRODUCTS=ON
cmake --build build --target email_center_api
./build/_deliverables/apps/email_center_api/email_center_api --port 8022
```

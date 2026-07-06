# call_center_api README

Path: `_deliverables/apps/call_center_api`

This backend service powers the call center browser interface.

## Related Interface

- Frontend: `_interfaces/call_center`
- Frontend entry logic: `_interfaces/call_center/src/main.js`

## What It Does

- Registers and unregisters SIP-style endpoints
- Manages presence updates for browser peers
- Relays WebRTC signaling messages
- Selects NAT candidates
- Configures and places PSTN calls through FreeSWITCH

## Main Routes

- `GET /api/voip/status`
- `POST /api/voip/sip/register`
- `POST /api/voip/sip/unregister`
- `POST /api/voip/mcu/join`
- `POST /api/voip/mcu/leave`
- `POST /api/voip/nat/select`
- `POST /api/voip/exposure`
- `POST /api/voip/presence/online`
- `POST /api/voip/presence/offline`
- `POST /api/voip/signals/send`
- `GET /api/voip/signals/poll?uri=sip:agent@coolbox.local`
- `GET /api/voip/pstn/config`
- `POST /api/voip/pstn/config`
- `POST /api/voip/pstn/call`

## Build And Run

```bash
cd /Users/solid/wkspace/CoolBox
cmake -S . -B build -DBUILD_PRODUCTS=ON
cmake --build build --target call_center_api
./build/_deliverables/apps/call_center_api/call_center_api --port 8011
```
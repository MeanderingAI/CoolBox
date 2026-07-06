# Call Center (Vite + C++ API)

Path: `_interfaces/call_center`

A full call-center softphone stack with:

- Browser frontend (Vite)
- Dedicated C++ signaling/control API service under `_deliverables/apps/call_center_api`
- C++ VOIP core libraries in `_deliverables/libraries/groups/audio_visual_group/VOIP`

## Features

- SIP-style endpoint registration/unregistration
- MCU room join/leave orchestration
- NAT candidate selection helper
- REST signaling queue for offer/answer/ICE/hangup/reject
- Browser-to-browser audio calling via WebRTC
- PSTN dial panel for FreeSWITCH config and E.164 outbound calling

## Backend service

Runs against the dedicated call_center API routes:

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

Service target:

- `call_center_api`

## Development

```bash
cd _interfaces/call_center
npm install
npm run dev

cd <repo-root>
cmake -S . -B build -DBUILD_PRODUCTS=ON
cmake --build build --target call_center_api
./build/_deliverables/apps/call_center_api/call_center_api --port 8011
```

Default ports:

- Vite: `5174`
- Call Center C++ API: `8011`

The Vite dev server proxies `/api` traffic to `http://localhost:8011`.

## FreeSWITCH PSTN Adapter

The backend now includes a native C++ FreeSWITCH adapter (`voip_freeswitch_adapter`) that talks to the FreeSWITCH Event Socket and submits `bgapi originate` calls.

1. Configure FreeSWITCH bridge settings:

```bash
curl -s -X POST http://127.0.0.1:8011/api/voip/pstn/config \
	-H 'Content-Type: application/json' \
	-d '{
		"event_socket_host":"127.0.0.1",
		"event_socket_port":8021,
		"event_socket_password":"ClueCon",
		"sip_profile":"external",
		"gateway":"my_trunk_gateway",
		"caller_id_number":"+12125550100"
	}'
```

2. Originate a PSTN call:

```bash
curl -s -X POST http://127.0.0.1:8011/api/voip/pstn/call \
	-H 'Content-Type: application/json' \
	-d '{
		"from_uri":"sip:agent01@coolbox.local",
		"to_number":"+12125551212",
		"timeout_seconds":30
	}'
```

Notes:

- `to_number` is normalized as E.164 (`+` and 8-15 digits).
- The API response includes `job_uuid` from FreeSWITCH `bgapi` for call tracking.
- FreeSWITCH and the configured gateway/trunk must be reachable from the API host.

## C++ VOIP core

The VOIP domain model is implemented in C++ libraries:

- `sip_server`
- `voip_mcu`
- `nat_traversal`
- `voip_api_exposure`
- `voip_freeswitch_adapter`

The current API service is a pure C++ host using `http_server` + `servlets`, with REST signaling queues for browser clients.

## Optional WASM helper

Emscripten bindings now include `voip_api_exposure_js` from:

- `_deliverables/libraries/bindings/emscripten_bindings/voip_api_exposure_bindings.cpp`

This exports `voip_rest_description()` and `voip_websocket_description()` for browser use when building with Emscripten.

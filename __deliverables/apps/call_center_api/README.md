# call_center_api GUI Guide

This document explains what the Call Center GUI elements do and how they map to the C++ API service.

## Interface Folder Paths

- Main call center frontend: _interfaces/call_center
- Frontend entry UI logic: _interfaces/call_center/src/main.js
- This backend service: _deliverables/apps/call_center_api

## What This API Is

call_center_api is the backend used by the browser app in _interfaces/call_center.
It provides REST endpoints for:

- SIP-style endpoint registration and presence
- WebRTC signaling relay (offer, answer, ICE, hangup, reject)
- NAT candidate selection helper
- PSTN bridge configuration and outbound phone dialing through FreeSWITCH

## GUI Panel Breakdown

The GUI has five functional cards.

### 1) Identity

Purpose:
- Define who the browser client is in SIP URI form.
- Register or unregister that endpoint in call_center_api.

Controls:
- SIP URI
- Transport
- Register + Connect
- Unregister Endpoint

Backend actions:
- POST /api/voip/sip/register
- POST /api/voip/sip/unregister
- POST /api/voip/presence/online
- POST /api/voip/presence/offline

Notes:
- Register + Connect also starts signal polling and prepares local audio capture.

### 2) Call Control

Purpose:
- Browser-to-browser call flow using SIP-style identities and WebRTC media.

Controls:
- Peer SIP URI
- Start Call
- Accept Incoming
- Reject Incoming
- Hang Up
- state indicator

Backend actions:
- POST /api/voip/signals/send (offer, answer, ice, hangup, reject)
- GET /api/voip/signals/poll?uri=<sip-uri>

Notes:
- Media is peer-to-peer WebRTC audio.
- call_center_api is used as the signaling/control plane.

### 3) PSTN Bridge

Purpose:
- Configure FreeSWITCH Event Socket connection details.
- Place outbound calls to normal phone numbers (E.164).

Controls:
- Event Socket Host
- Event Socket Port
- Event Socket Password
- SIP Profile
- Gateway
- Caller ID Number
- Load PSTN Config
- Save PSTN Config
- Phone Number (E.164)
- Timeout (seconds)
- Call Number
- pstn status indicator

Backend actions:
- GET /api/voip/pstn/config
- POST /api/voip/pstn/config
- POST /api/voip/pstn/call

Notes:
- The call endpoint normalizes and validates to_number as E.164.
- A successful originate returns call_id and job_uuid.
- If FreeSWITCH is unreachable on the configured socket, call attempts fail with a connection error.

### 4) NAT Traversal

Purpose:
- Compare local and reflexive candidates and choose the preferred one.

Controls:
- Local Candidate
- Reflexive Candidate
- Select Best Candidate
- result display

Backend action:
- POST /api/voip/nat/select

### 5) Exposure + Presence

Purpose:
- Select signaling exposure mode and refresh visibility/status.

Controls:
- Signaling Exposure
- Apply Exposure Mode
- Sync Presence
- Refresh Status
- online endpoints display
- local/remote audio elements
- event log

Backend actions:
- POST /api/voip/exposure
- POST /api/voip/presence/online
- GET /api/voip/status

## Quick Dev Notes

- Vite frontend typically runs from _interfaces/call_center.
- The API defaults to port 8011 unless overridden with --port.
- If you run multiple API instances, ensure frontend proxy target matches the intended backend port.

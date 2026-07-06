import './style.css';

const app = document.querySelector('#app');

app.innerHTML = `
  <h1>Call Center Softphone</h1>
  <p>Browser calling app using SIP-style registration, WebRTC media, and VOIP REST signaling on the C++ call_center_api service.</p>
  <div class="grid">
    <section class="card">
      <h3>Identity</h3>
      <label for="sip-uri">SIP URI</label>
      <input id="sip-uri" value="sip:agent01@coolbox.local" />
      <label for="sip-transport">Transport</label>
      <select id="sip-transport">
        <option value="ws">ws</option>
        <option value="wss">wss</option>
        <option value="udp">udp</option>
        <option value="tcp">tcp</option>
        <option value="tls">tls</option>
      </select>
      <button id="sip-register">Register + Connect</button>
      <button id="sip-unregister" class="danger">Unregister Endpoint</button>
    </section>

    <section class="card">
      <h3>Call Control</h3>
      <label for="peer-uri">Peer SIP URI</label>
      <input id="peer-uri" value="sip:agent02@coolbox.local" />
      <button id="start-call">Start Call</button>
      <button id="accept-call" class="alt">Accept Incoming</button>
      <button id="reject-call" class="danger">Reject Incoming</button>
      <button id="hangup-call" class="danger">Hang Up</button>
      <div class="status" id="call-state">state: idle</div>
    </section>

    <section class="card">
      <h3>PSTN Bridge</h3>
      <label for="fs-host">Event Socket Host</label>
      <input id="fs-host" value="127.0.0.1" />
      <label for="fs-port">Event Socket Port</label>
      <input id="fs-port" value="8021" />
      <label for="fs-password">Event Socket Password</label>
      <input id="fs-password" value="ClueCon" />
      <label for="fs-profile">SIP Profile</label>
      <input id="fs-profile" value="external" />
      <label for="fs-gateway">Gateway</label>
      <input id="fs-gateway" placeholder="my_trunk_gateway" />
      <label for="fs-caller">Caller ID Number</label>
      <input id="fs-caller" placeholder="+12125550100" />
      <button id="pstn-load" class="alt">Load PSTN Config</button>
      <button id="pstn-save">Save PSTN Config</button>

      <label for="pstn-number">Phone Number (E.164)</label>
      <input id="pstn-number" placeholder="+12125551212" />
      <label for="pstn-timeout">Timeout (seconds)</label>
      <input id="pstn-timeout" value="30" />
      <button id="pstn-call">Call Number</button>
      <div class="status" id="pstn-state">pstn: not configured</div>
    </section>

    <section class="card">
      <h3>NAT Traversal</h3>
      <label for="nat-local">Local Candidate</label>
      <input id="nat-local" value="192.168.1.16:5000" />
      <label for="nat-reflexive">Reflexive Candidate</label>
      <input id="nat-reflexive" value="203.0.113.10:46021" />
      <button id="nat-select">Select Best Candidate</button>
      <div class="status" id="nat-result"></div>
    </section>

    <section class="card">
      <h3>Exposure + Presence</h3>
      <label for="exposure">Signaling Exposure</label>
      <select id="exposure">
        <option value="rest">rest</option>
        <option value="websocket">websocket</option>
      </select>
      <button id="set-exposure">Apply Exposure Mode</button>
      <button id="sync-presence" class="alt">Sync Presence</button>
      <button id="refresh-status" class="alt">Refresh Status</button>
      <div class="status" id="presence">online: []</div>
      <audio id="remote-audio" autoplay playsinline></audio>
      <audio id="local-audio" autoplay muted playsinline></audio>
      <pre id="event-log"></pre>
    </section>
  </div>
`;

const logEl = document.querySelector('#event-log');
const natResultEl = document.querySelector('#nat-result');
const callStateEl = document.querySelector('#call-state');
const presenceEl = document.querySelector('#presence');
const localAudioEl = document.querySelector('#local-audio');
const remoteAudioEl = document.querySelector('#remote-audio');
const pstnStateEl = document.querySelector('#pstn-state');

const rtcConfig = {
  iceServers: [
    { urls: 'stun:stun.l.google.com:19302' },
    { urls: 'stun:stun.cloudflare.com:3478' },
  ],
};

let localStream = null;
let remoteStream = null;
let pc = null;
let pendingOffer = null;
let currentCallId = null;
let signalPollTimer = null;

function currentUri() {
  return document.querySelector('#sip-uri').value.trim();
}

function currentPeer() {
  return document.querySelector('#peer-uri').value.trim();
}

function setCallState(state) {
  callStateEl.textContent = `state: ${state}`;
}

function log(msg) {
  const timestamp = new Date().toISOString();
  logEl.textContent += `[${timestamp}] ${msg}\n`;
  logEl.scrollTop = logEl.scrollHeight;
}

function setPstnState(msg) {
  pstnStateEl.textContent = `pstn: ${msg}`;
}

async function api(path, method = 'GET', payload = null) {
  const opts = { method, headers: { 'Content-Type': 'application/json' } };
  if (payload) {
    opts.body = JSON.stringify(payload);
  }
  const res = await fetch(path, opts);
  const data = await res.json();
  if (!res.ok) {
    throw new Error(data.error || 'request failed');
  }
  return data;
}

function parseCandidate(value) {
  const [address, portRaw] = value.split(':');
  const port = Number(portRaw || 0);
  return { address: address || '', port, protocol: 'udp' };
}

async function ensureLocalMedia() {
  if (localStream) {
    return localStream;
  }
  localStream = await navigator.mediaDevices.getUserMedia({ audio: true, video: false });
  localAudioEl.srcObject = localStream;
  return localStream;
}

function createPeerConnection(peerUri) {
  if (pc) {
    pc.close();
  }

  pc = new RTCPeerConnection(rtcConfig);
  remoteStream = new MediaStream();
  remoteAudioEl.srcObject = remoteStream;

  pc.ontrack = (ev) => {
    ev.streams[0].getTracks().forEach((track) => remoteStream.addTrack(track));
    log(`remote track: ${trackSummary(ev.streams[0])}`);
  };

  pc.onicecandidate = (ev) => {
    if (!ev.candidate || !currentCallId) {
      return;
    }
    sendSignal({
      type: 'ice',
      from: currentUri(),
      to: peerUri,
      call_id: currentCallId,
      candidate: candidateToPayload(ev.candidate),
    }).catch((err) => log(`signal.ice.error => ${err.message}`));
  };

  pc.onconnectionstatechange = () => {
    setCallState(`webrtc.${pc.connectionState}`);
  };

  return pc;
}

function trackSummary(stream) {
  return `${stream.getTracks().map((t) => t.kind).join(',') || 'none'}`;
}

async function attachLocalTracks(connection) {
  const stream = await ensureLocalMedia();
  for (const track of stream.getTracks()) {
    connection.addTrack(track, stream);
  }
}

async function refreshStatus() {
  const status = await api('/api/voip/status');
  presenceEl.textContent = `online: ${JSON.stringify(status.online_endpoints || [])}`;
  setPstnState(status.pstn_configured ? 'configured' : 'not configured');
  log(`status => ${JSON.stringify(status)}`);
}

async function loadPstnConfig() {
  const out = await api('/api/voip/pstn/config');
  document.querySelector('#fs-host').value = out.event_socket_host || '';
  document.querySelector('#fs-port').value = String(out.event_socket_port || '');
  document.querySelector('#fs-password').value = '';
  document.querySelector('#fs-profile').value = out.sip_profile || '';
  document.querySelector('#fs-gateway').value = out.gateway || '';
  document.querySelector('#fs-caller').value = out.caller_id_number || '';
  setPstnState(out.configured ? 'configured' : 'not configured');
  log(`pstn.config.get => ${JSON.stringify(out)}`);
}

async function savePstnConfig() {
  const event_socket_host = document.querySelector('#fs-host').value.trim();
  const event_socket_port = Number(document.querySelector('#fs-port').value || 0);
  const event_socket_password = document.querySelector('#fs-password').value.trim();
  const sip_profile = document.querySelector('#fs-profile').value.trim();
  const gateway = document.querySelector('#fs-gateway').value.trim();
  const caller_id_number = document.querySelector('#fs-caller').value.trim();

  const payload = {
    event_socket_host,
    event_socket_port,
    event_socket_password,
    sip_profile,
    gateway,
    caller_id_number,
  };

  const out = await api('/api/voip/pstn/config', 'POST', payload);
  setPstnState(out.configured ? 'configured' : 'not configured');
  log(`pstn.config.set => ${JSON.stringify(out)}`);
}

async function callPstnNumber() {
  const from_uri = currentUri();
  const to_number = document.querySelector('#pstn-number').value.trim();
  const timeout_seconds = Number(document.querySelector('#pstn-timeout').value || 30);

  if (!to_number) {
    log('pstn.call => missing to_number');
    return;
  }

  const out = await api('/api/voip/pstn/call', 'POST', {
    from_uri,
    to_number,
    timeout_seconds,
  });

  currentCallId = out.call_id || null;
  setCallState(currentCallId ? `pstn.dialing (${currentCallId})` : 'pstn.dialing');
  log(`pstn.call => ${JSON.stringify(out)}`);
}

async function sendSignal(payload) {
  return api('/api/voip/signals/send', 'POST', payload);
}

function candidateToPayload(candidate) {
  if (!candidate) {
    return '';
  }
  return JSON.stringify(candidate);
}

function payloadToCandidate(candidatePayload) {
  if (!candidatePayload) {
    return null;
  }
  try {
    return JSON.parse(candidatePayload);
  } catch (_err) {
    return null;
  }
}

async function syncPresenceOnline() {
  const uri = currentUri();
  if (!uri) {
    return;
  }
  await api('/api/voip/presence/online', 'POST', { uri });
}

async function syncPresenceOffline() {
  const uri = currentUri();
  if (!uri) {
    return;
  }
  try {
    await api('/api/voip/presence/offline', 'POST', { uri });
  } catch (_err) {
    // Ignore shutdown races.
  }
}

async function pollSignals() {
  const uri = currentUri();
  if (!uri) {
    return;
  }

  let payload;
  try {
    payload = await api(`/api/voip/signals/poll?uri=${encodeURIComponent(uri)}`);
  } catch (err) {
    log(`signals.poll.error => ${err.message}`);
    return;
  }

  const messages = Array.isArray(payload.messages) ? payload.messages : [];
  for (const msg of messages) {
    await handleSignalMessage(msg);
  }
}

function startSignalPolling() {
  if (signalPollTimer) {
    window.clearInterval(signalPollTimer);
  }
  signalPollTimer = window.setInterval(() => {
    pollSignals().catch((err) => log(`signals.poll.exception => ${err.message}`));
  }, 500);
}

function stopSignalPolling() {
  if (signalPollTimer) {
    window.clearInterval(signalPollTimer);
    signalPollTimer = null;
  }
}

async function handleSignalMessage(msg) {
  if (msg.type === 'signal.offer') {
    let parsedOffer = null;
    if (msg.sdp) {
      try {
        parsedOffer = JSON.parse(msg.sdp);
      } catch (_err) {
        log('call.incoming => invalid SDP payload');
      }
    }
    pendingOffer = { from: msg.from, call_id: msg.call_id, sdp: parsedOffer };
    document.querySelector('#peer-uri').value = msg.from || '';
    currentCallId = msg.call_id;
    setCallState('incoming');
    log(`call.incoming => ${msg.from} (${msg.call_id})`);
    return;
  }

  if (msg.type === 'signal.answer') {
    if (!pc || !msg.sdp) {
      return;
    }
    let sdp;
    try {
      sdp = JSON.parse(msg.sdp);
    } catch (_err) {
      log('call.answer => invalid SDP payload');
      return;
    }
    await pc.setRemoteDescription(new RTCSessionDescription(sdp));
    currentCallId = msg.call_id;
    setCallState('active');
    log(`call.active => ${msg.call_id}`);
    return;
  }

  if (msg.type === 'signal.ice') {
    if (pc && msg.candidate) {
      const candidate = payloadToCandidate(msg.candidate);
      if (!candidate) {
        return;
      }
      try {
        await pc.addIceCandidate(candidate);
      } catch (err) {
        log(`ice.error => ${err.message}`);
      }
    }
    return;
  }

  if (msg.type === 'signal.hangup' || msg.type === 'signal.reject') {
    log(`call.end => ${msg.type} (${msg.reason || 'no-reason'})`);
    if (pc) {
      pc.close();
      pc = null;
    }
    pendingOffer = null;
    currentCallId = null;
    setCallState('idle');
    return;
  }
}

document.querySelector('#sip-register').addEventListener('click', async () => {
  const uri = currentUri();
  const transport = document.querySelector('#sip-transport').value;
  const out = await api('/api/voip/sip/register', 'POST', { uri, transport });
  log(`sip.register => ${JSON.stringify(out)}`);
  await syncPresenceOnline();
  startSignalPolling();
  await ensureLocalMedia();
});

document.querySelector('#sip-unregister').addEventListener('click', async () => {
  const uri = currentUri();
  const out = await api('/api/voip/sip/unregister', 'POST', { uri });
  log(`sip.unregister => ${JSON.stringify(out)}`);
  setCallState('idle');
  if (pc) {
    pc.close();
    pc = null;
  }
  stopSignalPolling();
  await syncPresenceOffline();
});

document.querySelector('#start-call').addEventListener('click', async () => {
  const from = currentUri();
  const to = currentPeer();
  if (!from || !to) {
    log('call.start => missing from/to URI');
    return;
  }
  const connection = createPeerConnection(to);
  await attachLocalTracks(connection);

  const offer = await connection.createOffer({ offerToReceiveAudio: true });
  await connection.setLocalDescription(offer);

  currentCallId = crypto.randomUUID();
  await sendSignal({
    type: 'offer',
    from,
    to,
    call_id: currentCallId,
    sdp: JSON.stringify(offer),
  });
  setCallState('dialing');
  log(`call.offer => ${from} -> ${to}`);
});

document.querySelector('#accept-call').addEventListener('click', async () => {
  if (!pendingOffer) {
    log('call.accept => no incoming offer');
    return;
  }
  const { from, call_id, sdp } = pendingOffer;
  if (!sdp) {
    log('call.accept => missing incoming SDP');
    return;
  }

  const connection = createPeerConnection(from);
  await attachLocalTracks(connection);
  await connection.setRemoteDescription(new RTCSessionDescription(sdp));

  const answer = await connection.createAnswer();
  await connection.setLocalDescription(answer);

  currentCallId = call_id;
  await sendSignal({
    type: 'answer',
    from: currentUri(),
    to: from,
    call_id,
    sdp: JSON.stringify(answer),
  });

  setCallState('active');
  log(`call.answer => accepted ${call_id}`);
  pendingOffer = null;
});

document.querySelector('#reject-call').addEventListener('click', async () => {
  if (!pendingOffer) {
    return;
  }
  await sendSignal({
    type: 'reject',
    from: currentUri(),
    to: pendingOffer.from,
    call_id: pendingOffer.call_id,
    reason: 'declined',
  });
  log(`call.reject => ${pendingOffer.call_id}`);
  setCallState('idle');
  pendingOffer = null;
});

document.querySelector('#hangup-call').addEventListener('click', async () => {
  if (!currentCallId) {
    return;
  }
  await sendSignal({
    type: 'hangup',
    from: currentUri(),
    to: currentPeer(),
    call_id: currentCallId,
    reason: 'hangup',
  });
  if (pc) {
    pc.close();
    pc = null;
  }
  currentCallId = null;
  setCallState('idle');
  log('call.hangup => local');
});

document.querySelector('#nat-select').addEventListener('click', async () => {
  const local = parseCandidate(document.querySelector('#nat-local').value.trim());
  const reflexive = parseCandidate(document.querySelector('#nat-reflexive').value.trim());
  const out = await api('/api/voip/nat/select', 'POST', { local, reflexive });
  natResultEl.textContent = `Chosen: ${out.candidate.address}:${out.candidate.port} (${out.candidate.protocol})`;
  log(`nat.select => ${JSON.stringify(out)}`);
});

document.querySelector('#set-exposure').addEventListener('click', async () => {
  const exposure = document.querySelector('#exposure').value;
  const out = await api('/api/voip/exposure', 'POST', { exposure });
  log(`exposure.set => ${JSON.stringify(out)}`);
});

function openWs() {
  startSignalPolling();
  log('signals.polling => active');
}

document.querySelector('#sync-presence').addEventListener('click', async () => {
  await syncPresenceOnline();
  openWs();
  await refreshStatus();
});

document.querySelector('#refresh-status').addEventListener('click', refreshStatus);
document.querySelector('#pstn-load').addEventListener('click', loadPstnConfig);
document.querySelector('#pstn-save').addEventListener('click', savePstnConfig);
document.querySelector('#pstn-call').addEventListener('click', callPstnNumber);

window.addEventListener('beforeunload', () => {
  syncPresenceOffline();
  stopSignalPolling();
});

refreshStatus()
  .then(() => loadPstnConfig())
  .then(() => ensureLocalMedia())
  .catch((err) => log(`startup.error => ${err.message}`));

import './style.css';

const app = document.querySelector('#app');

app.innerHTML = `
  <h1>Email Center</h1>
  <p>Browser UI for email_center_api. Register an account, send email, and view mailbox messages.</p>

  <div class="grid">
    <section class="card">
      <h3>Account</h3>
      <label for="account-address">Address</label>
      <input id="account-address" value="agent01@coolbox.local" />
      <label for="account-name">Display Name</label>
      <input id="account-name" value="Agent 01" />
      <label for="outbound-host">Outbound Host</label>
      <input id="outbound-host" value="smtp.coolbox.local" />
      <label for="outbound-port">Outbound Port</label>
      <input id="outbound-port" value="587" />
      <button id="register-account">Register Account</button>
      <button id="unregister-account" class="danger">Unregister Account</button>
      <div class="status" id="account-status">account: idle</div>
    </section>

    <section class="card">
      <h3>Compose</h3>
      <label for="mailbox">Mailbox</label>
      <input id="mailbox" value="inbox" />
      <label for="to">To</label>
      <input id="to" value="customer@example.com" />
      <label for="subject">Subject</label>
      <input id="subject" value="Hello from Email Center" />
      <label for="body">Body</label>
      <textarea id="body">This is a test message from _interfaces/email_center.</textarea>
      <button id="send-email">Send Email</button>
      <button id="refresh-messages" class="alt">Refresh Messages</button>
      <div class="status" id="send-status">send: idle</div>
    </section>

    <section class="card">
      <h3>Mailbox Messages</h3>
      <div class="status" id="mailbox-status">messages: 0</div>
      <div id="message-list" class="list"></div>
    </section>

    <section class="card">
      <h3>Service Status</h3>
      <button id="refresh-status" class="alt">Refresh Service Status</button>
      <pre id="status-json"></pre>
      <pre id="event-log"></pre>
    </section>
  </div>
`;

const accountStatusEl = document.querySelector('#account-status');
const sendStatusEl = document.querySelector('#send-status');
const mailboxStatusEl = document.querySelector('#mailbox-status');
const statusJsonEl = document.querySelector('#status-json');
const eventLogEl = document.querySelector('#event-log');
const messageListEl = document.querySelector('#message-list');

function accountAddress() {
  return document.querySelector('#account-address').value.trim();
}

function mailboxName() {
  return document.querySelector('#mailbox').value.trim();
}

function log(msg) {
  const ts = new Date().toISOString();
  eventLogEl.textContent += `[${ts}] ${msg}\n`;
  eventLogEl.scrollTop = eventLogEl.scrollHeight;
}

function setAccountStatus(msg) {
  accountStatusEl.textContent = `account: ${msg}`;
}

function setSendStatus(msg) {
  sendStatusEl.textContent = `send: ${msg}`;
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

function renderMessages(messages) {
  if (!Array.isArray(messages) || messages.length === 0) {
    messageListEl.innerHTML = '<div class="item"><h4>No messages</h4><div class="meta">Mailbox is empty.</div></div>';
    mailboxStatusEl.textContent = 'messages: 0';
    return;
  }

  mailboxStatusEl.textContent = `messages: ${messages.length}`;
  messageListEl.innerHTML = messages
    .map((m) => {
      const subject = escapeHtml(m.subject || '(no subject)');
      const from = escapeHtml(m.from || 'unknown');
      const to = escapeHtml(m.to || 'unknown');
      const ts = escapeHtml(m.timestamp || 'n/a');
      const body = escapeHtml(m.body || '');
      const read = m.read ? 'read' : 'unread';
      const id = escapeHtml(m.id || '');
      return `
        <div class="item">
          <h4>${subject}</h4>
          <div class="meta">${from} -> ${to}</div>
          <div class="meta">id=${id} | ${ts} | ${read}</div>
          <div class="body">${body}</div>
        </div>
      `;
    })
    .join('');
}

function escapeHtml(value) {
  return String(value)
    .replaceAll('&', '&amp;')
    .replaceAll('<', '&lt;')
    .replaceAll('>', '&gt;')
    .replaceAll('"', '&quot;')
    .replaceAll("'", '&#39;');
}

async function refreshServiceStatus() {
  const out = await api('/api/email/status');
  statusJsonEl.textContent = JSON.stringify(out, null, 2);
  log(`status => ${JSON.stringify(out)}`);
}

async function refreshMessages() {
  const mailbox = mailboxName();
  if (!mailbox) {
    throw new Error('mailbox is required');
  }
  const out = await api(`/api/email/messages?mailbox=${encodeURIComponent(mailbox)}`);
  renderMessages(out.messages || []);
  log(`messages => mailbox=${mailbox} count=${(out.messages || []).length}`);
}

async function registerAccount() {
  const payload = {
    address: accountAddress(),
    display_name: document.querySelector('#account-name').value.trim(),
    outbound_host: document.querySelector('#outbound-host').value.trim(),
    outbound_port: Number(document.querySelector('#outbound-port').value || 587),
  };
  const out = await api('/api/email/smtp/register', 'POST', payload);
  setAccountStatus('registered');
  log(`account.register => ${JSON.stringify(out)}`);
}

async function unregisterAccount() {
  const out = await api('/api/email/smtp/unregister', 'POST', {
    address: accountAddress(),
  });
  setAccountStatus('unregistered');
  log(`account.unregister => ${JSON.stringify(out)}`);
}

async function ensureMailbox(mailbox) {
  if (!mailbox) {
    throw new Error('mailbox is required');
  }
  try {
    await api('/api/email/mailbox/create', 'POST', { mailbox });
    log(`mailbox.create => ${mailbox}`);
  } catch (err) {
    // mailbox may already exist; keep flow going.
    log(`mailbox.create.skip => ${err.message}`);
  }
}

async function sendEmail() {
  const mailbox = mailboxName();
  await ensureMailbox(mailbox);

  const payload = {
    from_account: accountAddress(),
    mailbox,
    to: document.querySelector('#to').value.trim(),
    subject: document.querySelector('#subject').value.trim(),
    body: document.querySelector('#body').value,
  };

  const out = await api('/api/email/send', 'POST', payload);
  setSendStatus('sent');
  log(`send => ${JSON.stringify(out)}`);
  await refreshMessages();
  await refreshServiceStatus();
}

document.querySelector('#register-account').addEventListener('click', () => {
  registerAccount().catch((err) => {
    setAccountStatus(`error (${err.message})`);
    log(`account.register.error => ${err.message}`);
  });
});

document.querySelector('#unregister-account').addEventListener('click', () => {
  unregisterAccount().catch((err) => {
    setAccountStatus(`error (${err.message})`);
    log(`account.unregister.error => ${err.message}`);
  });
});

document.querySelector('#send-email').addEventListener('click', () => {
  sendEmail().catch((err) => {
    setSendStatus(`error (${err.message})`);
    log(`send.error => ${err.message}`);
  });
});

document.querySelector('#refresh-messages').addEventListener('click', () => {
  refreshMessages().catch((err) => log(`messages.error => ${err.message}`));
});

document.querySelector('#refresh-status').addEventListener('click', () => {
  refreshServiceStatus().catch((err) => log(`status.error => ${err.message}`));
});

Promise.resolve()
  .then(() => refreshServiceStatus())
  .then(() => refreshMessages())
  .catch((err) => log(`startup.error => ${err.message}`));

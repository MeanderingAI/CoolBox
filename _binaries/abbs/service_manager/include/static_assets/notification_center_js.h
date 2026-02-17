// Auto-generated from notification-center.js
#pragma once

namespace resources {
    constexpr const char* notification_center_js = R"JS(
class NotificationCenter extends HTMLElement {
  constructor() {
    super();
    this.attachShadow({mode: 'open'});
    this.notifications = [];
    this.open = false;
    this.render();
  }
  connectedCallback() {
    this.render();
  }
  addNotification(msg, type = 'info') {
    this.notifications.push({msg, type, time: new Date()});
    this.render();
  }
  toggleOpen() {
    this.open = !this.open;
    this.render();
  }
  render() {
    this.shadowRoot.innerHTML = `
      <style>
        :host { position: fixed; bottom: 24px; right: 24px; z-index: 9999; }
        .fab {
          background: linear-gradient(90deg, #6366f1 0%, #8b5cf6 100%);
          color: #fff;
          border: none;
          border-radius: 50%;
          width: 56px; height: 56px;
          font-size: 2rem;
          box-shadow: 0 2px 8px rgba(139,92,246,0.18);
          cursor: pointer;
          display: flex; align-items: center; justify-content: center;
          transition: background 0.2s;
        }
        .panel {
          position: absolute; bottom: 70px; right: 0;
          min-width: 320px; max-width: 400px;
          background: #232336;
          border-radius: 12px;
          box-shadow: 0 4px 24px rgba(0,0,0,0.18);
          padding: 18px 16px 12px 16px;
          color: #f3f3f3;
          display: ${this.open ? 'block' : 'none'};
        }
        .notif {
          margin-bottom: 12px;
          padding: 10px 12px;
          border-radius: 6px;
          background: #2d2d44;
          font-size: 1rem;
          border-left: 4px solid #6366f1;
        }
        .notif.info { border-color: #6366f1; }
        .notif.error { border-color: #ef4444; background: #3b1f1f; }
        .notif.success { border-color: #22c55e; background: #1f3b2d; }
        .notif .time { float: right; font-size: 0.85em; color: #a78bfa; margin-left: 8px; }
        .notif:last-child { margin-bottom: 0; }
        .panel h3 { margin: 0 0 10px 0; font-size: 1.1em; color: #a78bfa; }
      </style>
      <button class="fab" title="Notifications" onclick="this.getRootNode().host.toggleOpen()">🔔</button>
      <div class="panel">
        <h3>Notifications</h3>
        ${this.notifications.length ? this.notifications.map(n => `<div class="notif ${n.type}">${n.msg}<span class="time">${n.time.toLocaleTimeString()}</span></div>`).join('') : '<div class="notif">No notifications</div>'}
      </div>
    `;
  }
}
customElements.define('notification-center', NotificationCenter);

)JS";
}

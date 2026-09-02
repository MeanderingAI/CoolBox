(function(){const t=document.createElement("link").relList;if(t&&t.supports&&t.supports("modulepreload"))return;for(const a of document.querySelectorAll('link[rel="modulepreload"]'))r(a);new MutationObserver(a=>{for(const l of a)if(l.type==="childList")for(const g of l.addedNodes)g.tagName==="LINK"&&g.rel==="modulepreload"&&r(g)}).observe(document,{childList:!0,subtree:!0});function n(a){const l={};return a.integrity&&(l.integrity=a.integrity),a.referrerPolicy&&(l.referrerPolicy=a.referrerPolicy),a.crossOrigin==="use-credentials"?l.credentials="include":a.crossOrigin==="anonymous"?l.credentials="omit":l.credentials="same-origin",l}function r(a){if(a.ep)return;a.ep=!0;const l=n(a);fetch(a.href,l)}})();const L=document.querySelector("#app");L.innerHTML=`
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
`;const S=document.querySelector("#event-log"),x=document.querySelector("#nat-result"),I=document.querySelector("#call-state"),R=document.querySelector("#presence"),J=document.querySelector("#local-audio"),D=document.querySelector("#remote-audio"),j=document.querySelector("#pstn-state"),A={iceServers:[{urls:"stun:stun.l.google.com:19302"},{urls:"stun:stun.cloudflare.com:3478"}]};let y=null,v=null,i=null,d=null,c=null,f=null;function u(){return document.querySelector("#sip-uri").value.trim()}function P(){return document.querySelector("#peer-uri").value.trim()}function p(e){I.textContent=`state: ${e}`}function o(e){const t=new Date().toISOString();S.textContent+=`[${t}] ${e}
`,S.scrollTop=S.scrollHeight}function b(e){j.textContent=`pstn: ${e}`}async function s(e,t="GET",n=null){const r={method:t,headers:{"Content-Type":"application/json"}};n&&(r.body=JSON.stringify(n));const a=await fetch(e,r),l=await a.json();if(!a.ok)throw new Error(l.error||"request failed");return l}function _(e){const[t,n]=e.split(":"),r=Number(n||0);return{address:t||"",port:r,protocol:"udp"}}async function w(){return y||(y=await navigator.mediaDevices.getUserMedia({audio:!0,video:!1}),J.srcObject=y,y)}function O(e){return i&&i.close(),i=new RTCPeerConnection(A),v=new MediaStream,D.srcObject=v,i.ontrack=t=>{t.streams[0].getTracks().forEach(n=>v.addTrack(n)),o(`remote track: ${U(t.streams[0])}`)},i.onicecandidate=t=>{!t.candidate||!c||m({type:"ice",from:u(),to:e,call_id:c,candidate:B(t.candidate)}).catch(n=>o(`signal.ice.error => ${n.message}`))},i.onconnectionstatechange=()=>{p(`webrtc.${i.connectionState}`)},i}function U(e){return`${e.getTracks().map(t=>t.kind).join(",")||"none"}`}async function C(e){const t=await w();for(const n of t.getTracks())e.addTrack(n,t)}async function h(){const e=await s("/api/voip/status");R.textContent=`online: ${JSON.stringify(e.online_endpoints||[])}`,b(e.pstn_configured?"configured":"not configured"),o(`status => ${JSON.stringify(e)}`)}async function T(){const e=await s("/api/voip/pstn/config");document.querySelector("#fs-host").value=e.event_socket_host||"",document.querySelector("#fs-port").value=String(e.event_socket_port||""),document.querySelector("#fs-password").value="",document.querySelector("#fs-profile").value=e.sip_profile||"",document.querySelector("#fs-gateway").value=e.gateway||"",document.querySelector("#fs-caller").value=e.caller_id_number||"",b(e.configured?"configured":"not configured"),o(`pstn.config.get => ${JSON.stringify(e)}`)}async function M(){const e=document.querySelector("#fs-host").value.trim(),t=Number(document.querySelector("#fs-port").value||0),n=document.querySelector("#fs-password").value.trim(),r=document.querySelector("#fs-profile").value.trim(),a=document.querySelector("#fs-gateway").value.trim(),l=document.querySelector("#fs-caller").value.trim(),q=await s("/api/voip/pstn/config","POST",{event_socket_host:e,event_socket_port:t,event_socket_password:n,sip_profile:r,gateway:a,caller_id_number:l});b(q.configured?"configured":"not configured"),o(`pstn.config.set => ${JSON.stringify(q)}`)}async function H(){const e=u(),t=document.querySelector("#pstn-number").value.trim(),n=Number(document.querySelector("#pstn-timeout").value||30);if(!t){o("pstn.call => missing to_number");return}const r=await s("/api/voip/pstn/call","POST",{from_uri:e,to_number:t,timeout_seconds:n});c=r.call_id||null,p(c?`pstn.dialing (${c})`:"pstn.dialing"),o(`pstn.call => ${JSON.stringify(r)}`)}async function m(e){return s("/api/voip/signals/send","POST",e)}function B(e){return e?JSON.stringify(e):""}function G(e){if(!e)return null;try{return JSON.parse(e)}catch{return null}}async function $(){const e=u();e&&await s("/api/voip/presence/online","POST",{uri:e})}async function E(){const e=u();if(e)try{await s("/api/voip/presence/offline","POST",{uri:e})}catch{}}async function W(){const e=u();if(!e)return;let t;try{t=await s(`/api/voip/signals/poll?uri=${encodeURIComponent(e)}`)}catch(r){o(`signals.poll.error => ${r.message}`);return}const n=Array.isArray(t.messages)?t.messages:[];for(const r of n)await F(r)}function k(){f&&window.clearInterval(f),f=window.setInterval(()=>{W().catch(e=>o(`signals.poll.exception => ${e.message}`))},500)}function N(){f&&(window.clearInterval(f),f=null)}async function F(e){if(e.type==="signal.offer"){let t=null;if(e.sdp)try{t=JSON.parse(e.sdp)}catch{o("call.incoming => invalid SDP payload")}d={from:e.from,call_id:e.call_id,sdp:t},document.querySelector("#peer-uri").value=e.from||"",c=e.call_id,p("incoming"),o(`call.incoming => ${e.from} (${e.call_id})`);return}if(e.type==="signal.answer"){if(!i||!e.sdp)return;let t;try{t=JSON.parse(e.sdp)}catch{o("call.answer => invalid SDP payload");return}await i.setRemoteDescription(new RTCSessionDescription(t)),c=e.call_id,p("active"),o(`call.active => ${e.call_id}`);return}if(e.type==="signal.ice"){if(i&&e.candidate){const t=G(e.candidate);if(!t)return;try{await i.addIceCandidate(t)}catch(n){o(`ice.error => ${n.message}`)}}return}if(e.type==="signal.hangup"||e.type==="signal.reject"){o(`call.end => ${e.type} (${e.reason||"no-reason"})`),i&&(i.close(),i=null),d=null,c=null,p("idle");return}}document.querySelector("#sip-register").addEventListener("click",async()=>{const e=u(),t=document.querySelector("#sip-transport").value,n=await s("/api/voip/sip/register","POST",{uri:e,transport:t});o(`sip.register => ${JSON.stringify(n)}`),await $(),k(),await w()});document.querySelector("#sip-unregister").addEventListener("click",async()=>{const e=u(),t=await s("/api/voip/sip/unregister","POST",{uri:e});o(`sip.unregister => ${JSON.stringify(t)}`),p("idle"),i&&(i.close(),i=null),N(),await E()});document.querySelector("#start-call").addEventListener("click",async()=>{const e=u(),t=P();if(!e||!t){o("call.start => missing from/to URI");return}const n=O(t);await C(n);const r=await n.createOffer({offerToReceiveAudio:!0});await n.setLocalDescription(r),c=crypto.randomUUID(),await m({type:"offer",from:e,to:t,call_id:c,sdp:JSON.stringify(r)}),p("dialing"),o(`call.offer => ${e} -> ${t}`)});document.querySelector("#accept-call").addEventListener("click",async()=>{if(!d){o("call.accept => no incoming offer");return}const{from:e,call_id:t,sdp:n}=d;if(!n){o("call.accept => missing incoming SDP");return}const r=O(e);await C(r),await r.setRemoteDescription(new RTCSessionDescription(n));const a=await r.createAnswer();await r.setLocalDescription(a),c=t,await m({type:"answer",from:u(),to:e,call_id:t,sdp:JSON.stringify(a)}),p("active"),o(`call.answer => accepted ${t}`),d=null});document.querySelector("#reject-call").addEventListener("click",async()=>{d&&(await m({type:"reject",from:u(),to:d.from,call_id:d.call_id,reason:"declined"}),o(`call.reject => ${d.call_id}`),p("idle"),d=null)});document.querySelector("#hangup-call").addEventListener("click",async()=>{c&&(await m({type:"hangup",from:u(),to:P(),call_id:c,reason:"hangup"}),i&&(i.close(),i=null),c=null,p("idle"),o("call.hangup => local"))});document.querySelector("#nat-select").addEventListener("click",async()=>{const e=_(document.querySelector("#nat-local").value.trim()),t=_(document.querySelector("#nat-reflexive").value.trim()),n=await s("/api/voip/nat/select","POST",{local:e,reflexive:t});x.textContent=`Chosen: ${n.candidate.address}:${n.candidate.port} (${n.candidate.protocol})`,o(`nat.select => ${JSON.stringify(n)}`)});document.querySelector("#set-exposure").addEventListener("click",async()=>{const e=document.querySelector("#exposure").value,t=await s("/api/voip/exposure","POST",{exposure:e});o(`exposure.set => ${JSON.stringify(t)}`)});function K(){k(),o("signals.polling => active")}document.querySelector("#sync-presence").addEventListener("click",async()=>{await $(),K(),await h()});document.querySelector("#refresh-status").addEventListener("click",h);document.querySelector("#pstn-load").addEventListener("click",T);document.querySelector("#pstn-save").addEventListener("click",M);document.querySelector("#pstn-call").addEventListener("click",H);window.addEventListener("beforeunload",()=>{E(),N()});h().then(()=>T()).then(()=>w()).catch(e=>o(`startup.error => ${e.message}`));

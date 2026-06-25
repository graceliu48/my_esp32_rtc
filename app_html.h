#ifndef APP_HTML_H
#define APP_HTML_H

#include <Arduino.h>

// index.html
static const char APP_INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
<meta name="theme-color" content="#1a1a2e">
<title>ESP32 RTC</title>
<link rel="manifest" href="manifest.json">
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
body { font-family: -apple-system, 'Segoe UI', sans-serif; background: #1a1a2e; color: #eee; min-height: 100vh; display: flex; flex-direction: column; }
header { background: linear-gradient(135deg, #16213e, #0f3460); padding: 20px 16px; text-align: center; position: relative; }
header h1 { font-size: 18px; font-weight: 600; color: #e94560; letter-spacing: 2px; }
.conn-bar { display: flex; align-items: center; justify-content: center; gap: 8px; margin-top: 8px; font-size: 12px; color: #aaa; }
.conn-bar .dot { width: 8px; height: 8px; border-radius: 50%; display: inline-block; }
.conn-bar .dot.online { background: #4ecca3; box-shadow: 0 0 6px #4ecca3; }
.conn-bar .dot.offline { background: #e94560; box-shadow: 0 0 6px #e94560; }
.clock { background: linear-gradient(135deg, #0f3460, #16213e); margin: 16px; border-radius: 16px; padding: 24px; text-align: center; box-shadow: 0 4px 20px rgba(0,0,0,.4); }
.clock .time { font-size: 52px; font-weight: 700; font-variant-numeric: tabular-nums; letter-spacing: 2px; color: #fff; }
.clock .date { font-size: 16px; color: #aaa; margin-top: 6px; }
.clock .weekday { display: inline-block; margin-top: 8px; padding: 4px 16px; background: #e94560; border-radius: 12px; font-size: 13px; font-weight: 500; }
.clock .uptime { font-size: 11px; color: #666; margin-top: 10px; }
.grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; padding: 0 16px; margin-bottom: 16px; }
.card { background: #16213e; border-radius: 12px; padding: 14px; }
.card.full { grid-column: 1 / -1; }
.card .label { font-size: 11px; color: #666; text-transform: uppercase; letter-spacing: 1px; margin-bottom: 4px; }
.card .value { font-size: 14px; color: #eee; word-break: break-all; }
.card .value.small { font-size: 12px; }
.controls { padding: 0 16px 16px; }
.controls h3 { font-size: 14px; color: #888; margin-bottom: 10px; letter-spacing: 1px; }
.btn-row { display: flex; gap: 8px; flex-wrap: wrap; margin-bottom: 12px; }
.btn { flex: 1; min-width: 60px; padding: 10px 14px; border: none; border-radius: 10px; font-size: 13px; font-weight: 500; cursor: pointer; transition: .2s; color: #fff; }
.btn:active { transform: scale(.95); }
.btn.primary { background: #0f3460; }
.btn.danger { background: #e94560; }
.btn.success { background: #4ecca3; color: #1a1a2e; }
.btn.warning { background: #e9a945; color: #1a1a2e; }
.btn.outline { background: transparent; border: 1px solid #333; }
.slider-row { display: flex; align-items: center; gap: 12px; padding: 8px 0; }
.slider-row label { font-size: 12px; color: #888; min-width: 56px; }
.slider-row input[type=range] { flex: 1; accent-color: #e94560; }
.slider-row .val { font-size: 14px; color: #eee; min-width: 32px; text-align: right; }
.toggle-row { display: flex; align-items: center; justify-content: space-between; padding: 8px 0; }
.toggle-row span { font-size: 13px; color: #ccc; }
.toggle { width: 48px; height: 26px; background: #333; border-radius: 13px; position: relative; cursor: pointer; transition: .2s; }
.toggle.active { background: #4ecca3; }
.toggle::after { content: ''; position: absolute; top: 2px; left: 2px; width: 22px; height: 22px; background: #fff; border-radius: 50%; transition: .2s; }
.toggle.active::after { left: 24px; }
.modal { display: none; position: fixed; inset: 0; background: rgba(0,0,0,.6); z-index: 100; align-items: center; justify-content: center; padding: 20px; }
.modal.show { display: flex; }
.modal-content { background: #16213e; border-radius: 16px; padding: 24px; width: 100%; max-width: 360px; }
.modal-content h3 { margin-bottom: 16px; font-size: 16px; color: #eee; }
.modal-content input { width: 100%; padding: 10px 12px; background: #0f3460; border: 1px solid #333; border-radius: 8px; color: #eee; font-size: 14px; margin-bottom: 8px; }
.modal-content input:focus { outline: none; border-color: #e94560; }
.modal-actions { display: flex; gap: 8px; margin-top: 12px; }
.toast { position: fixed; bottom: 20px; left: 50%; transform: translateX(-50%); background: #333; color: #fff; padding: 10px 20px; border-radius: 8px; font-size: 13px; z-index: 200; opacity: 0; transition: .3s; pointer-events: none; }
.toast.show { opacity: 1; }
@media (min-width: 500px) { .container { max-width: 440px; margin: 0 auto; } }
</style>
</head>
<body>
<div class="container">
<header>
  <h1>ESP32 RTC</h1>
  <div class="conn-bar">
    <span id="connDot" class="dot offline"></span>
    <span id="connText">未连接</span>
    <span style="color:#555">|</span>
    <span id="deviceIp">-</span>
  </div>
</header>
<div class="clock">
  <div class="time" id="clockTime">--:--:--</div>
  <div class="date" id="clockDate">----年--月--日</div>
  <div class="weekday" id="clockWeekday">---</div>
  <div class="uptime" id="uptimeText">运行时间: --秒</div>
</div>
<div class="grid">
  <div class="card"><div class="label">WiFi</div><div class="value small" id="wifiStatus">-</div></div>
  <div class="card"><div class="label">LCD</div><div class="value small" id="lcdStatus">-</div></div>
  <div class="card full"><div class="label">亮度</div><div class="value small" id="brightnessVal">-</div></div>
</div>
<div class="controls">
  <h3>控制</h3>
  <div class="toggle-row">
    <span>LCD 屏幕</span>
    <div class="toggle" id="lcdToggle" onclick="toggleLcd()"></div>
  </div>
  <div class="slider-row">
    <label>亮度</label>
    <input type="range" min="0" max="100" value="50" id="brightnessSlider" oninput="setBrightness(this.value)">
    <span class="val" id="brightnessLabel">50</span>
  </div>
  <div class="btn-row" style="margin-top:12px">
    <button class="btn success" onclick="syncNtp()">NTP 同步</button>
    <button class="btn warning" onclick="showTimeModal()">手动校时</button>
  </div>
  <div class="btn-row">
    <button class="btn danger" onclick="restart()">重启设备</button>
    <button class="btn outline" onclick="showWifiModal()">WiFi 配置</button>
  </div>
</div>
</div>
<div class="modal" id="timeModal">
  <div class="modal-content">
    <h3>手动校时</h3>
    <input type="datetime-local" id="manualDatetime">
    <div class="modal-actions">
      <button class="btn primary" style="flex:1" onclick="setTime()">确认</button>
      <button class="btn outline" style="flex:1" onclick="closeModal('timeModal')">取消</button>
    </div>
  </div>
</div>
<div class="modal" id="wifiModal">
  <div class="modal-content">
    <h3>WiFi 配置</h3>
    <input type="text" id="wifiSsid" placeholder="SSID">
    <input type="password" id="wifiPass" placeholder="密码">
    <div class="modal-actions">
      <button class="btn primary" style="flex:1" onclick="setWifi()">保存并重启</button>
      <button class="btn outline" style="flex:1" onclick="closeModal('wifiModal')">取消</button>
    </div>
  </div>
</div>
<div class="toast" id="toast"></div>
<script>
let baseUrl = '';
let refreshTimer = null;
function toast(m) { const t=document.getElementById('toast'); t.textContent=m; t.className='toast show'; setTimeout(()=>t.className='toast',2000); }
async function api(method, path, body) {
  try {
    const opts = { method, headers: {} };
    if (body) { opts.headers['Content-Type']='application/json'; opts.body=JSON.stringify(body); }
    const r = await fetch(baseUrl + path, opts);
    return await r.json();
  } catch(e) {
    document.getElementById('connDot').className='dot offline';
    document.getElementById('connText').textContent='连接失败';
    return null;
  }
}
async function fetchStatus() {
  const d = await api('GET','/api/status');
  if (!d) return;
  document.getElementById('wifiStatus').textContent = d.wifi==='connected' ? d.ssid+' ('+d.ip+')' : '未连接';
  document.getElementById('lcdStatus').textContent = d.lcd ? '已开启' : '已关闭';
  document.getElementById('brightnessVal').textContent = d.brightness+'%';
  document.getElementById('lcdToggle').className = d.lcd ? 'toggle active' : 'toggle';
  document.getElementById('brightnessSlider').value = d.brightness;
  document.getElementById('brightnessLabel').textContent = d.brightness;
  document.getElementById('connDot').className='dot online';
  document.getElementById('connText').textContent='已连接';
  document.getElementById('deviceIp').textContent=d.ip;
}
async function fetchTime() {
  const d = await api('GET','/api/time');
  if (!d) return;
  if (d.synced) {
    document.getElementById('clockTime').textContent = d.time;
    document.getElementById('clockDate').textContent = d.date.replace(/-/g,'/');
    document.getElementById('clockWeekday').textContent = d.weekday;
  } else {
    document.getElementById('clockTime').textContent='--:--:--';
    document.getElementById('clockDate').textContent='未同步';
    document.getElementById('clockWeekday').textContent='---';
  }
  if (d.uptime!==undefined) {
    const h=Math.floor(d.uptime/3600), m=Math.floor((d.uptime%3600)/60), s=d.uptime%60;
    document.getElementById('uptimeText').textContent='运行时间: '+h+'时'+m+'分'+s+'秒';
  }
}
async function toggleLcd() {
  const on = !document.getElementById('lcdToggle').className.includes('active');
  const d = await api('POST','/api/lcd',{on});
  if (d&&d.ok) { document.getElementById('lcdToggle').className=d.lcd?'toggle active':'toggle'; document.getElementById('lcdStatus').textContent=d.lcd?'已开启':'已关闭'; toast(d.lcd?'LCD 已开启':'LCD 已关闭'); }
  else toast('操作失败');
}
let bd = null;
function setBrightness(v) {
  document.getElementById('brightnessLabel').textContent=v;
  clearTimeout(bd); bd=setTimeout(async()=>{ const d=await api('POST','/api/brightness',{value:parseInt(v)}); if(d&&d.ok) toast('亮度: '+d.brightness+'%'); },300);
}
async function syncNtp() { const d=await api('POST','/api/sync',{}); if(d&&d.ok) toast('NTP 同步已触发'); else toast('同步失败'); }
function showTimeModal() {
  const n=new Date(); document.getElementById('manualDatetime').value=new Date(n.getTime()-n.getTimezoneOffset()*60000).toISOString().slice(0,16);
  document.getElementById('timeModal').className='modal show';
}
async function setTime() {
  const v=document.getElementById('manualDatetime').value; if(!v) return;
  const d=await api('POST','/api/time',{datetime:v.replace('T',' ')+':00'});
  if(d&&d.ok) { toast('校时成功'); document.getElementById('timeModal').className='modal'; fetchTime(); } else toast('校时失败');
}
function showWifiModal() { document.getElementById('wifiSsid').value=''; document.getElementById('wifiPass').value=''; document.getElementById('wifiModal').className='modal show'; }
function closeModal(id) { document.getElementById(id).className='modal'; }
async function setWifi() {
  const s=document.getElementById('wifiSsid').value.trim(), p=document.getElementById('wifiPass').value;
  if(!s) { toast('请输入 SSID'); return; }
  const d=await api('POST','/api/wifi',{ssid:s,password:p});
  if(d&&d.ok) { toast('WiFi 已配置，设备重启中...'); setTimeout(()=>document.getElementById('wifiModal').className='modal',500); } else toast('配置失败');
}
function restart() { if(!confirm('确定重启设备？')) return; api('POST','/api/restart',{}).then(d=>{ if(d&&d.ok) { toast('设备重启中...'); setTimeout(()=>location.reload(),5000); }}); }
function initConnection() {
  if (window.location.protocol==='http:'||window.location.protocol==='https:') { baseUrl=''; }
  else { const s=localStorage.getItem('esp_ip'); baseUrl=s?'http://'+s:'http://dnesp32s3m.local'; }
  fetchStatus(); fetchTime(); refreshTimer=setInterval(()=>{ fetchTime(); fetchStatus(); },3000);
}
if (window.location.protocol!=='http:'&&window.location.protocol!=='https:') {
  if (!localStorage.getItem('esp_ip')) { const ip=prompt('输入 ESP32 IP，留空用 dnesp32s3m.local:'); if(ip&&ip.trim()) localStorage.setItem('esp_ip',ip.trim()); }
}
document.addEventListener('DOMContentLoaded', initConnection);
</script>
</body>
</html>
)rawliteral";

// manifest.json
static const char APP_MANIFEST_JSON[] PROGMEM = R"rawliteral({
  "name": "ESP32 RTC",
  "short_name": "ESP32 RTC",
  "start_url": "/",
  "display": "standalone",
  "background_color": "#1a1a2e",
  "theme_color": "#1a1a2e",
  "icons": [
    {
      "src": "data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'><rect width='100' height='100' rx='20' fill='%231a1a2e'/><text x='50' y='68' font-size='50' text-anchor='middle' fill='%23e94560'>⏰</text></svg>",
      "sizes": "any",
      "type": "image/svg+xml",
      "purpose": "any maskable"
    }
  ]
}
)rawliteral";

// sw.js
static const char APP_SW_JS[] PROGMEM = R"rawliteral(self.addEventListener('install',()=>self.skipWaiting());
self.addEventListener('activate',e=>e.waitUntil(clients.claim()));
self.addEventListener('fetch',e=>e.respondWith(fetch(e.request)));
)rawliteral";

#endif

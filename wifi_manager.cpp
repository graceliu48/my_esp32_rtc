#include "wifi_manager.h"
#include "rtc_config.h"

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="mobile-web-app-capable" content="yes">
<link rel="manifest" href="/manifest.json">
<title>DNESP32S3M 配网</title>
<style>
*{margin:0;padding:0;box-sizing:border-box;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif}
body{background:#1a1a2e;min-height:100vh;display:flex;justify-content:center;align-items:center;padding:16px}
.card{background:#16213e;border-radius:16px;padding:24px;width:100%;max-width:400px;box-shadow:0 8px 32px rgba(0,0,0,.3)}
h1{color:#e94560;font-size:20px;text-align:center;margin-bottom:4px}
.sub{color:#8899aa;font-size:12px;text-align:center;margin-bottom:20px}
label{color:#ccd;font-size:13px;display:block;margin:12px 0 4px}
.sel-wrap{display:flex;gap:8px}
select{flex:1;background:#0f3460;color:#fff;border:1px solid #1a5276;border-radius:8px;padding:10px 12px;font-size:14px;outline:none}
.btn{background:#e94560;color:#fff;border:none;border-radius:8px;padding:10px 16px;font-size:14px;cursor:pointer;white-space:nowrap}
.btn:active{opacity:.8}
.btn-scan{background:#0f3460;padding:10px}
.btn-save{width:100%;margin-top:16px;padding:12px;font-size:15px;font-weight:600}
.btn-del{background:#533;width:100%;margin-top:8px;padding:10px;font-size:13px}
input[type=password]{width:100%;background:#0f3460;color:#fff;border:1px solid #1a5276;border-radius:8px;padding:10px 12px;font-size:14px;outline:none;margin-top:4px}
input[type=password]:focus,select:focus{border-color:#e94560}
#status{padding:10px;border-radius:8px;margin:12px 0 0;font-size:13px;display:none}
#status.ok{display:block;background:#0d3320;color:#4caf50;border:1px solid #2e7d32}
#status.err{display:block;background:#330d0d;color:#ef5350;border:1px solid #7d2e2e}
#status.info{display:block;background:#0d1f33;color:#42a5f5;border:1px solid #1a5276}
#status.wifi{display:block;background:#0d3320;color:#66bb6a;border:1px solid #2e7d32}
.loading{text-align:center;padding:20px;color:#667}
.loading::after{content:"";display:inline-block;width:20px;height:20px;border:3px solid #334;border-top-color:#e94560;border-radius:50%;animation:spin .8s linear infinite;margin-left:8px;vertical-align:middle}
@keyframes spin{to{transform:rotate(360deg)}}
</style>
</head>
<body>
<div class="card">
<h1>DNESP32S3M RTC</h1>
<p class="sub">WiFi 配网</p>
<div id="status"></div>
<label>WiFi 网络</label>
<div class="sel-wrap">
<select id="ssid" onchange="pw.focus()"><option value="">-- 扫描中 --</option></select>
<button class="btn btn-scan" onclick="doScan()">扫描</button>
</div>
<label>密码</label>
<input type="password" id="pw" placeholder="输入WiFi密码">
<button class="btn btn-save" onclick="doSave()">保存并连接</button>
<button class="btn btn-del" onclick="doReset()">清除配置</button>
</div>
<script>
function status(m,t){var s=document.getElementById('status');s.className=t;s.innerHTML=m;if(t!='info'){setTimeout(function(){s.style.display='none'},5000)}}
function doScan(){var s=document.getElementById('ssid');s.innerHTML='<option>-- 扫描中 --</option>';status('扫描中...','info');var x=new XMLHttpRequest();x.onload=function(){if(x.status==200){var j=JSON.parse(x.responseText);s.innerHTML='<option value="">-- 请选择网络 --</option>';j.networks.forEach(function(n){var o=document.createElement('option');o.value=n.ssid;var e=n.enc?'🔒 ':'🔓 ';o.text=e+n.ssid+' ('+n.rssi+'dBm)';s.appendChild(o)});if(j.networks.length==0){s.innerHTML='<option value="">-- 未扫描到网络 --</option>'}}else{status('扫描失败','err')}};x.onerror=function(){status('请求失败','err')};x.open('GET','/scan',true);x.send()}
function doSave(){var s=document.getElementById('ssid').value;var p=document.getElementById('pw').value;if(!s){status('请选择WiFi网络','err');return}status('连接中,请等待...','info');var x=new XMLHttpRequest();x.onload=function(){if(x.status==200){var j=JSON.parse(x.responseText);if(j.ok){status('保存成功!设备将连接 '+j.ssid+' 并重启','ok')}else{status('保存失败: '+j.msg,'err')}}else{status('请求失败','err')}};x.onerror=function(){status('网络错误','err')};x.open('POST','/save',true);x.setRequestHeader('Content-Type','application/x-www-form-urlencoded');x.send('ssid='+encodeURIComponent(s)+'&password='+encodeURIComponent(p))}
function doReset(){if(!confirm('清除WiFi配置并重启?'))return;var x=new XMLHttpRequest();x.onload=function(){if(x.status==200){status('已清除,设备将重启','info');setTimeout(function(){location.reload()},2000)}};x.open('GET','/reset',true);x.send()}
setTimeout(doScan,500);
</script>
</body>
</html>
)rawliteral";

static const char MANIFEST_JSON[] PROGMEM = R"rawliteral({
  "name":"DNESP32S3M RTC Config",
  "short_name":"RTC Config",
  "display":"standalone",
  "background_color":"#1a1a2e",
  "theme_color":"#e94560",
  "start_url":"/"
})rawliteral";

WiFiManager::WiFiManager()
  : _server(80), _configMode(false), _serverStarted(false),
    _hasStoredCreds(false) {
}

void WiFiManager::begin() {
  WiFi.persistent(false);
  loadCredentials();

  uint8_t mac[6];
  WiFi.macAddress(mac);
  char apName[32];
  snprintf(apName, sizeof(apName), "DNESP32S3M_%02X%02X",
           mac[4], mac[5]);
  _apSSID = String(apName);
}

void WiFiManager::loadCredentials() {
  _prefs.begin("wifi", true);
  _storedSSID = _prefs.getString("ssid", "");
  _storedPassword = _prefs.getString("password", "");
  _prefs.end();
  _hasStoredCreds = _storedSSID.length() > 0;
}

void WiFiManager::saveCredentials(const String &ssid, const String &password) {
  _prefs.begin("wifi", false);
  _prefs.putString("ssid", ssid);
  _prefs.putString("password", password);
  _prefs.end();
  _storedSSID = ssid;
  _storedPassword = password;
  _hasStoredCreds = true;
}

void WiFiManager::clearCredentials() {
  _prefs.begin("wifi", false);
  _prefs.remove("ssid");
  _prefs.remove("password");
  _prefs.end();
  _storedSSID = "";
  _storedPassword = "";
  _hasStoredCreds = false;
}

bool WiFiManager::connectToWiFi(int timeout_ms) {
  String ssid = _hasStoredCreds ? _storedSSID : WIFI_SSID;
  String pass = _hasStoredCreds ? _storedPassword : WIFI_PASSWORD;

  Serial.printf("[WiFi] 连接 %s\n", ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long start = millis();
  while (millis() - start < (unsigned long)timeout_ms) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf(" OK (%s)\n", WiFi.localIP().toString().c_str());
      return true;
    }
    delay(100);
    Serial.print(".");
  }
  Serial.println(" 超时");
  return false;
}

bool WiFiManager::startConfigMode() {
  if (_configMode) return true;
  _configMode = true;
  startAP();
  startWebServer();
  return true;
}

void WiFiManager::stopConfigMode() {
  if (!_configMode) return;
  _configMode = false;
  _server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
}

void WiFiManager::startAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1),
                    IPAddress(255,255,255,0));
  WiFi.softAP(_apSSID.c_str(), NULL, 1, 0, 1);
  Serial.printf("[AP] 启动 %s (192.168.4.1)\n", _apSSID.c_str());
}

void WiFiManager::startWebServer() {
  _server.on("/", [this]() { handleRoot(); });
  _server.on("/scan", [this]() { handleScan(); });
  _server.on("/save", HTTP_POST, [this]() { handleSave(); });
  _server.on("/manifest.json", [this]() { handleManifest(); });
  _server.on("/reset", [this]() {
    clearCredentials();
    _server.send(200, "text/plain", "OK");
    delay(1000);
    ESP.restart();
  });
  _server.begin();
  _serverStarted = true;
}

void WiFiManager::loop() {
  if (_configMode && _serverStarted) {
    _server.handleClient();
  }
}

void WiFiManager::handleRoot() {
  _server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void WiFiManager::handleManifest() {
  _server.send_P(200, "application/manifest+json", MANIFEST_JSON);
}

void WiFiManager::handleScan() {
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_FAILED) {
    WiFi.scanNetworks(true);
    _server.send(200, "application/json", "{\"networks\":[]}");
    return;
  }
  if (n == WIFI_SCAN_RUNNING) {
    _server.send(200, "application/json", "{\"networks\":[]}");
    return;
  }

  String json = "{\"networks\":[";
  for (int i = 0; i < n; i++) {
    if (i > 0) json += ",";
    json += "{\"ssid\":\"";
    String ssid = WiFi.SSID(i);
    ssid.replace("\"", "\\\"");
    json += ssid;
    json += "\",\"rssi\":";
    json += WiFi.RSSI(i);
    json += ",\"enc\":";
    json += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? 0 : 1;
    json += "}";
  }
  json += "]}";
  WiFi.scanDelete();

  _server.send(200, "application/json; charset=utf-8", json);
}

void WiFiManager::handleSave() {
  if (!_server.hasArg("ssid")) {
    _server.send(200, "application/json",
                 "{\"ok\":false,\"msg\":\"缺少SSID\"}");
    return;
  }

  String ssid = _server.arg("ssid");
  String password = _server.arg("password");
  ssid.trim();
  if (ssid.length() == 0) {
    _server.send(200, "application/json",
                 "{\"ok\":false,\"msg\":\"SSID不能为空\"}");
    return;
  }

  saveCredentials(ssid, password);

  String resp = "{\"ok\":true,\"ssid\":\"";
  resp += ssid;
  resp += "\"}";
  _server.send(200, "application/json; charset=utf-8", resp);

  Serial.printf("[Config] 保存: %s\n", ssid.c_str());
  delay(500);
  ESP.restart();
}

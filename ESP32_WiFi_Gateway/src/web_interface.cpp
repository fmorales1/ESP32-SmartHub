#include "web_interface.h"
#include "config.h"
#include <ArduinoJson.h>

// Referência global ao GatewayManager
GatewayManager* _netManager = nullptr;
WebServer* _webServer = nullptr;

// Handlers das rotas
void handleRoot();
void handleDevices();
void handleSettings();
void handleSaveSettings();
void handleStatus();
void handleReboot();
void handleNotFound();

void setupWebInterface(WebServer& server, GatewayManager& netManager) {
    _netManager = &netManager;
    _webServer = &server;
    
    // Páginas principais
    server.on("/", HTTP_GET, handleRoot);
    server.on("/devices", HTTP_GET, handleDevices);
    server.on("/settings", HTTP_GET, handleSettings);
    server.on("/save", HTTP_POST, handleSaveSettings);
    
    // API endpoints
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/reboot", HTTP_POST, handleReboot);
    
    // 404
    server.onNotFound(handleNotFound);
}

void handleRoot() {
    String html = getMainPage(*_netManager);
    _webServer->send(200, "text/html", html);
}

void handleDevices() {
    String html = getDevicesPage(*_netManager);
    _webServer->send(200, "text/html", html);
}

void handleSettings() {
    String html = getSettingsPage(*_netManager);
    _webServer->send(200, "text/html", html);
}

void handleSaveSettings() {
    // Processa configurações salvas
    // (implementação futura com EEPROM/Preferences)
    _webServer->send(200, "text/html", 
        "<html><body><h2>Configurações salvas!</h2>"
        "<p>Reinicie o dispositivo para aplicar.</p>"
        "<a href='/'>Voltar</a></body></html>");
}

void handleStatus() {
    StaticJsonDocument<512> doc;
    
    doc["uptime"] = _netManager->getUptimeSeconds();
    doc["wifi_connected"] = _netManager->isConnectedToWiFi();
    doc["wifi_rssi"] = _netManager->getStationRSSI();
    doc["wifi_ip"] = _netManager->getStationIP().toString();
    doc["ap_running"] = _netManager->isAPRunning();
    doc["ap_ip"] = _netManager->getAPIP().toString();
    doc["ap_clients"] = _netManager->getConnectedDevicesCount();
    doc["nat_enabled"] = _netManager->isNATEnabled();
    doc["free_heap"] = ESP.getFreeHeap();
    
    String response;
    serializeJson(doc, response);
    _webServer->send(200, "application/json", response);
}

void handleReboot() {
    _webServer->send(200, "text/plain", "Reiniciando...");
    delay(1000);
    ESP.restart();
}

void handleNotFound() {
    // Redireciona para página principal (captive portal)
    _webServer->sendHeader("Location", "/", true);
    _webServer->send(302, "text/plain", "");
}

String getStyleCSS() {
    return R"(
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body { 
            font-family: 'Segoe UI', Arial, sans-serif; 
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            min-height: 100vh;
            color: #eee;
        }
        .container { max-width: 800px; margin: 0 auto; padding: 20px; }
        .header {
            text-align: center;
            padding: 30px 0;
            border-bottom: 1px solid #333;
            margin-bottom: 30px;
        }
        .header h1 { 
            color: #00d4ff; 
            font-size: 2em;
            text-shadow: 0 0 20px rgba(0,212,255,0.5);
        }
        .header p { color: #888; margin-top: 10px; }
        .nav {
            display: flex;
            justify-content: center;
            gap: 20px;
            margin-bottom: 30px;
        }
        .nav a {
            color: #00d4ff;
            text-decoration: none;
            padding: 10px 20px;
            border: 1px solid #00d4ff;
            border-radius: 5px;
            transition: all 0.3s;
        }
        .nav a:hover, .nav a.active {
            background: #00d4ff;
            color: #1a1a2e;
        }
        .card {
            background: rgba(255,255,255,0.05);
            border-radius: 15px;
            padding: 25px;
            margin-bottom: 20px;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255,255,255,0.1);
        }
        .card h2 {
            color: #00d4ff;
            margin-bottom: 20px;
            font-size: 1.3em;
        }
        .status-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
        }
        .status-item {
            background: rgba(0,0,0,0.3);
            padding: 15px;
            border-radius: 10px;
            text-align: center;
        }
        .status-item .label { color: #888; font-size: 0.9em; }
        .status-item .value { 
            font-size: 1.5em; 
            font-weight: bold;
            color: #00ff88;
            margin-top: 5px;
        }
        .status-item .value.warning { color: #ffaa00; }
        .status-item .value.error { color: #ff4444; }
        .device-list { list-style: none; }
        .device-list li {
            display: flex;
            justify-content: space-between;
            padding: 15px;
            background: rgba(0,0,0,0.2);
            margin-bottom: 10px;
            border-radius: 8px;
            align-items: center;
        }
        .device-icon { font-size: 1.5em; margin-right: 15px; }
        .device-info { flex-grow: 1; }
        .device-ip { color: #00d4ff; font-family: monospace; }
        .device-mac { color: #888; font-size: 0.85em; }
        .online-badge {
            background: #00ff88;
            color: #000;
            padding: 3px 10px;
            border-radius: 20px;
            font-size: 0.8em;
        }
        .btn {
            background: #00d4ff;
            color: #1a1a2e;
            border: none;
            padding: 12px 25px;
            border-radius: 8px;
            cursor: pointer;
            font-size: 1em;
            transition: all 0.3s;
        }
        .btn:hover { background: #00ff88; }
        .btn-danger { background: #ff4444; color: white; }
        .btn-danger:hover { background: #ff6666; }
        input, select {
            width: 100%;
            padding: 12px;
            margin: 10px 0;
            border: 1px solid #333;
            border-radius: 8px;
            background: rgba(0,0,0,0.3);
            color: #eee;
            font-size: 1em;
        }
        input:focus, select:focus {
            outline: none;
            border-color: #00d4ff;
        }
        label { color: #888; display: block; margin-top: 15px; }
        .form-group { margin-bottom: 20px; }
        .info-box {
            background: rgba(0,212,255,0.1);
            border-left: 4px solid #00d4ff;
            padding: 15px;
            margin: 20px 0;
            border-radius: 0 8px 8px 0;
        }
        .footer {
            text-align: center;
            padding: 20px;
            color: #666;
            font-size: 0.9em;
        }
    )";
}

String getMainPage(GatewayManager& netManager) {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>ESP32 WiFi Gateway</title>";
    html += "<style>" + getStyleCSS() + "</style>";
    html += "</head><body>";
    
    html += "<div class='container'>";
    html += "<div class='header'>";
    html += "<h1>🌐 ESP32 WiFi Gateway</h1>";
    html += "<p>Smart Home Network Isolation</p>";
    html += "</div>";
    
    // Navegação
    html += "<div class='nav'>";
    html += "<a href='/' class='active'>Status</a>";
    html += "<a href='/devices'>Dispositivos</a>";
    html += "<a href='/settings'>Configurações</a>";
    html += "</div>";
    
    // Card de Status WiFi Principal
    html += "<div class='card'>";
    html += "<h2>📡 Rede Principal</h2>";
    html += "<div class='status-grid'>";
    
    html += "<div class='status-item'>";
    html += "<div class='label'>Status</div>";
    html += "<div class='value" + String(netManager.isConnectedToWiFi() ? "" : " error") + "'>";
    html += netManager.isConnectedToWiFi() ? "Conectado" : "Desconectado";
    html += "</div></div>";
    
    html += "<div class='status-item'>";
    html += "<div class='label'>SSID</div>";
    html += "<div class='value'>" + netManager.getStationSSID() + "</div>";
    html += "</div>";
    
    html += "<div class='status-item'>";
    html += "<div class='label'>IP</div>";
    html += "<div class='value'>" + netManager.getStationIP().toString() + "</div>";
    html += "</div>";
    
    html += "<div class='status-item'>";
    html += "<div class='label'>Sinal</div>";
    int rssi = netManager.getStationRSSI();
    String rssiClass = rssi > -60 ? "" : (rssi > -80 ? " warning" : " error");
    html += "<div class='value" + rssiClass + "'>" + String(rssi) + " dBm</div>";
    html += "</div>";
    
    html += "</div></div>";
    
    // Card do Access Point
    html += "<div class='card'>";
    html += "<h2>📶 Rede SmartThings</h2>";
    html += "<div class='status-grid'>";
    
    html += "<div class='status-item'>";
    html += "<div class='label'>Status</div>";
    html += "<div class='value'>Ativo</div>";
    html += "</div>";
    
    html += "<div class='status-item'>";
    html += "<div class='label'>SSID</div>";
    html += "<div class='value'>" + netManager.getAPSSID() + "</div>";
    html += "</div>";
    
    html += "<div class='status-item'>";
    html += "<div class='label'>IP Gateway</div>";
    html += "<div class='value'>" + netManager.getAPIP().toString() + "</div>";
    html += "</div>";
    
    html += "<div class='status-item'>";
    html += "<div class='label'>Dispositivos</div>";
    html += "<div class='value'>" + String(netManager.getConnectedDevicesCount()) + "</div>";
    html += "</div>";
    
    html += "</div></div>";
    
    // Card de Sistema
    html += "<div class='card'>";
    html += "<h2>⚙️ Sistema</h2>";
    html += "<div class='status-grid'>";
    
    html += "<div class='status-item'>";
    html += "<div class='label'>Uptime</div>";
    unsigned long uptime = netManager.getUptimeSeconds();
    int hours = uptime / 3600;
    int minutes = (uptime % 3600) / 60;
    html += "<div class='value'>" + String(hours) + "h " + String(minutes) + "m</div>";
    html += "</div>";
    
    html += "<div class='status-item'>";
    html += "<div class='label'>Memória Livre</div>";
    html += "<div class='value'>" + String(ESP.getFreeHeap() / 1024) + " KB</div>";
    html += "</div>";
    
    html += "<div class='status-item'>";
    html += "<div class='label'>NAT</div>";
    html += "<div class='value" + String(netManager.isNATEnabled() ? "" : " warning") + "'>";
    html += netManager.isNATEnabled() ? "Ativo" : "Inativo";
    html += "</div></div>";
    
    html += "<div class='status-item'>";
    html += "<div class='label'>Chip</div>";
    html += "<div class='value'>ESP32</div>";
    html += "</div>";
    
    html += "</div></div>";
    
    // Info box
    html += "<div class='info-box'>";
    html += "<strong>💡 Dica:</strong> Conecte suas lâmpadas e dispositivos smart à rede '";
    html += netManager.getAPSSID() + "' para isolá-los da sua rede principal.";
    html += "</div>";
    
    // Footer
    html += "<div class='footer'>";
    html += "ESP32 WiFi Gateway v1.0 | Feito com ❤️";
    html += "</div>";
    
    html += "</div>";
    
    // Auto-refresh
    html += "<script>setTimeout(() => location.reload(), 30000);</script>";
    
    html += "</body></html>";
    return html;
}

String getDevicesPage(GatewayManager& netManager) {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>Dispositivos - ESP32 Gateway</title>";
    html += "<style>" + getStyleCSS() + "</style>";
    html += "</head><body>";
    
    html += "<div class='container'>";
    html += "<div class='header'>";
    html += "<h1>🌐 ESP32 WiFi Gateway</h1>";
    html += "<p>Dispositivos Conectados</p>";
    html += "</div>";
    
    html += "<div class='nav'>";
    html += "<a href='/'>Status</a>";
    html += "<a href='/devices' class='active'>Dispositivos</a>";
    html += "<a href='/settings'>Configurações</a>";
    html += "</div>";
    
    html += "<div class='card'>";
    html += "<h2>📱 Dispositivos na Rede SmartThings</h2>";
    
    auto devices = netManager.getConnectedDevices();
    
    if (devices.size() == 0) {
        html += "<p style='text-align:center; color:#888; padding:40px;'>";
        html += "Nenhum dispositivo conectado no momento.<br><br>";
        html += "Conecte seus dispositivos smart à rede:<br>";
        html += "<strong style='color:#00d4ff;'>" + netManager.getAPSSID() + "</strong>";
        html += "</p>";
    } else {
        html += "<ul class='device-list'>";
        
        for (auto& device : devices) {
            html += "<li>";
            html += "<span class='device-icon'>💡</span>";
            html += "<div class='device-info'>";
            html += "<div class='device-ip'>" + device.ip.toString() + "</div>";
            
            // Formata MAC
            char macStr[18];
            snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                     device.mac[0], device.mac[1], device.mac[2],
                     device.mac[3], device.mac[4], device.mac[5]);
            html += "<div class='device-mac'>" + String(macStr) + "</div>";
            
            html += "</div>";
            html += "<span class='online-badge'>Online</span>";
            html += "</li>";
        }
        
        html += "</ul>";
    }
    
    html += "</div>";
    
    html += "<div class='footer'>";
    html += "ESP32 WiFi Gateway v1.0";
    html += "</div>";
    
    html += "</div></body></html>";
    return html;
}

String getSettingsPage(GatewayManager& netManager) {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>Configurações - ESP32 Gateway</title>";
    html += "<style>" + getStyleCSS() + "</style>";
    html += "</head><body>";
    
    html += "<div class='container'>";
    html += "<div class='header'>";
    html += "<h1>🌐 ESP32 WiFi Gateway</h1>";
    html += "<p>Configurações do Sistema</p>";
    html += "</div>";
    
    html += "<div class='nav'>";
    html += "<a href='/'>Status</a>";
    html += "<a href='/devices'>Dispositivos</a>";
    html += "<a href='/settings' class='active'>Configurações</a>";
    html += "</div>";
    
    // Configurações da Rede Principal
    html += "<div class='card'>";
    html += "<h2>📡 Rede Principal (Internet)</h2>";
    html += "<form action='/save' method='POST'>";
    
    html += "<div class='form-group'>";
    html += "<label>SSID da sua rede WiFi</label>";
    html += "<input type='text' name='sta_ssid' value='" + netManager.getStationSSID() + "' placeholder='Nome da rede'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label>Senha</label>";
    html += "<input type='password' name='sta_pass' placeholder='Senha da rede'>";
    html += "</div>";
    
    html += "</form></div>";
    
    // Configurações do Access Point
    html += "<div class='card'>";
    html += "<h2>📶 Rede SmartThings (Dispositivos IoT)</h2>";
    
    html += "<div class='form-group'>";
    html += "<label>Nome da Rede (SSID)</label>";
    html += "<input type='text' name='ap_ssid' value='" + netManager.getAPSSID() + "' placeholder='SmartThings'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label>Senha (mínimo 8 caracteres)</label>";
    html += "<input type='password' name='ap_pass' placeholder='Nova senha'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label>Canal WiFi</label>";
    html += "<select name='ap_channel'>";
    for (int i = 1; i <= 13; i++) {
        html += "<option value='" + String(i) + "'>" + String(i) + "</option>";
    }
    html += "</select>";
    html += "</div>";
    
    html += "<button type='submit' class='btn'>Salvar Configurações</button>";
    html += "</div>";
    
    // Ações do Sistema
    html += "<div class='card'>";
    html += "<h2>🔧 Ações do Sistema</h2>";
    html += "<p style='margin-bottom:20px; color:#888;'>Reinicie o dispositivo para aplicar novas configurações.</p>";
    html += "<button onclick='reboot()' class='btn btn-danger'>Reiniciar ESP32</button>";
    html += "</div>";
    
    html += "<div class='footer'>";
    html += "ESP32 WiFi Gateway v1.0";
    html += "</div>";
    
    html += "</div>";
    
    // JavaScript
    html += "<script>";
    html += "function reboot() {";
    html += "  if(confirm('Deseja reiniciar o ESP32?')) {";
    html += "    fetch('/api/reboot', {method:'POST'}).then(() => {";
    html += "      alert('Reiniciando... Aguarde alguns segundos.');";
    html += "    });";
    html += "  }";
    html += "}";
    html += "</script>";
    
    html += "</body></html>";
    return html;
}

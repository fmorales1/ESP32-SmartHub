/*
 * ═══════════════════════════════════════════════════════════════
 * ESP32 IPTV Server - Servidor Web (Implementação)
 * ═══════════════════════════════════════════════════════════════
 */

#include "web_server.h"
#include "storage_manager.h"
#include "playlist_parser.h"
#include "wifi_manager.h"
#include <HTTPClient.h>

// Instância global
WebServerManager WebServer;

// Variável para acumular dados do upload
static File uploadFile;
static size_t uploadedSize = 0;
static bool uploadError = false;
static String uploadErrorMsg = "";

// ═══════════════════════════════════════════════════════════════
// CONSTRUTOR
// ═══════════════════════════════════════════════════════════════

WebServerManager::WebServerManager() {
  _server = nullptr;
  _running = false;
}

// ═══════════════════════════════════════════════════════════════
// INICIALIZAÇÃO
// ═══════════════════════════════════════════════════════════════

void WebServerManager::begin() {
  DEBUG_PRINTLN("[WebServer] Iniciando servidor web...");
  
  // Cria instância do servidor na porta configurada
  _server = new AsyncWebServer(WEB_SERVER_PORT);
  
  // Configura as rotas
  setupRoutes();
  
  // Inicia o servidor
  _server->begin();
  _running = true;
  
  DEBUG_PRINTF("[WebServer] Servidor iniciado na porta %d\n", WEB_SERVER_PORT);
}

void WebServerManager::stop() {
  if (_server != nullptr && _running) {
    _server->end();
    delete _server;
    _server = nullptr;
    _running = false;
    DEBUG_PRINTLN("[WebServer] Servidor parado.");
  }
}

bool WebServerManager::isRunning() {
  return _running;
}

// ═══════════════════════════════════════════════════════════════
// CONFIGURAÇÃO DE ROTAS
// ═══════════════════════════════════════════════════════════════

void WebServerManager::setupRoutes() {
  // Página principal
  _server->on("/", HTTP_GET, handleRoot);
  
  // Recursos estáticos
  _server->on("/style.css", HTTP_GET, handleCSS);
  _server->on("/script.js", HTTP_GET, handleJS);
  
  // Upload de arquivo M3U8
  _server->on("/upload", HTTP_POST, 
    [](AsyncWebServerRequest* request) {
      // Resposta após o upload
      StaticJsonDocument<256> doc;
      
      if (uploadError) {
        doc["success"] = false;
        doc["message"] = uploadErrorMsg;
      } else {
        // Faz parse da playlist carregada
        if (Playlist.parseFile(PLAYLIST_FILENAME)) {
          doc["success"] = true;
          doc["message"] = "Playlist carregada com sucesso!";
          doc["channels"] = Playlist.getChannelCount();
          doc["size"] = uploadedSize;
        } else {
          doc["success"] = false;
          doc["message"] = "Erro ao processar playlist: " + Playlist.getErrorMessage();
        }
      }
      
      String response;
      serializeJson(doc, response);
      request->send(200, "application/json", response);
      
      // Reset variáveis
      uploadError = false;
      uploadErrorMsg = "";
      uploadedSize = 0;
    },
    handleUpload
  );
  
  // Playlist bruta
  _server->on("/playlist.m3u8", HTTP_GET, handlePlaylist);
  
  // Lista JSON de canais
  _server->on("/list.json", HTTP_GET, handleListJSON);
  
  // Status do sistema
  _server->on("/api/status", HTTP_GET, handleStatus);
  
  // Scan WiFi
  _server->on("/api/wifi/scan", HTTP_GET, handleWiFiScan);
  
  // Conectar WiFi (com body JSON)
  _server->on("/api/wifi/connect", HTTP_POST, 
    [](AsyncWebServerRequest* request) {},
    NULL,
    handleWiFiConnect
  );
  
  // Proxy
  _server->on("/proxy", HTTP_GET, handleProxy);
  
  // Deletar playlist
  _server->on("/playlist", HTTP_DELETE, handleDeletePlaylist);
  
  // 404 Not Found
  _server->onNotFound(handleNotFound);
  
  DEBUG_PRINTLN("[WebServer] Rotas configuradas.");
}

// ═══════════════════════════════════════════════════════════════
// HANDLERS
// ═══════════════════════════════════════════════════════════════

void WebServerManager::handleRoot(AsyncWebServerRequest* request) {
  DEBUG_PRINTLN("[WebServer] GET /");
  request->send(200, "text/html", getHTML());
}

void WebServerManager::handleCSS(AsyncWebServerRequest* request) {
  request->send(200, "text/css", getCSS());
}

void WebServerManager::handleJS(AsyncWebServerRequest* request) {
  request->send(200, "application/javascript", getJS());
}

void WebServerManager::handleUpload(AsyncWebServerRequest* request, 
                                     String filename, 
                                     size_t index, 
                                     uint8_t* data, 
                                     size_t len, 
                                     bool final) {
  // Início do upload
  if (index == 0) {
    DEBUG_PRINTF("[WebServer] Upload iniciado: %s\n", filename.c_str());
    
    // Valida extensão
    if (!filename.endsWith(".m3u8") && !filename.endsWith(".m3u")) {
      uploadError = true;
      uploadErrorMsg = "Extensão inválida. Use .m3u ou .m3u8";
      return;
    }
    
    // Abre arquivo para escrita
    uploadFile = Storage.openFileForWrite(PLAYLIST_FILENAME);
    if (!uploadFile) {
      uploadError = true;
      uploadErrorMsg = "Erro ao criar arquivo";
      return;
    }
    
    uploadedSize = 0;
    uploadError = false;
  }
  
  // Escreve dados
  if (uploadFile && len > 0 && !uploadError) {
    // Verifica limite de tamanho
    if (uploadedSize + len > MAX_PLAYLIST_SIZE) {
      uploadError = true;
      uploadErrorMsg = "Arquivo muito grande (máximo 2MB)";
      uploadFile.close();
      Storage.deleteFile(PLAYLIST_FILENAME);
      return;
    }
    
    size_t written = uploadFile.write(data, len);
    uploadedSize += written;
    
    if (written != len) {
      uploadError = true;
      uploadErrorMsg = "Erro de escrita no arquivo";
    }
  }
  
  // Fim do upload
  if (final) {
    if (uploadFile) {
      uploadFile.close();
    }
    
    if (!uploadError) {
      DEBUG_PRINTF("[WebServer] Upload concluído: %d bytes\n", uploadedSize);
    }
  }
}

void WebServerManager::handlePlaylist(AsyncWebServerRequest* request) {
  DEBUG_PRINTLN("[WebServer] GET /playlist.m3u8");
  
  if (!Storage.fileExists(PLAYLIST_FILENAME)) {
    request->send(404, "text/plain", "Playlist não encontrada");
    return;
  }
  
  request->send(FILESYSTEM, PLAYLIST_FILENAME, "application/vnd.apple.mpegurl");
}

void WebServerManager::handleListJSON(AsyncWebServerRequest* request) {
  DEBUG_PRINTLN("[WebServer] GET /list.json");
  
  // Se a playlist não foi parseada, tenta carregar
  if (Playlist.getChannelCount() == 0 && Storage.fileExists(PLAYLIST_FILENAME)) {
    Playlist.parseFile(PLAYLIST_FILENAME);
  }
  
  String json = Playlist.toJSON();
  request->send(200, "application/json", json);
}

void WebServerManager::handleStatus(AsyncWebServerRequest* request) {
  DEBUG_PRINTLN("[WebServer] GET /api/status");
  
  StaticJsonDocument<512> doc;
  
  // Uptime em segundos
  doc["uptime"] = millis() / 1000;
  
  // RAM livre
  doc["free_ram"] = ESP.getFreeHeap();
  doc["total_ram"] = ESP.getHeapSize();
  
  // Tamanho da playlist
  doc["file_size"] = Storage.getFileSize(PLAYLIST_FILENAME);
  doc["file_exists"] = Storage.fileExists(PLAYLIST_FILENAME);
  
  // Número de canais
  doc["channel_count"] = Playlist.getChannelCount();
  
  // Informações de rede
  WiFiStatus wifiStatus = WifiMgr.getStatus();
  doc["ip"] = WifiMgr.getMainIP();
  doc["hostname"] = String(MDNS_HOSTNAME) + ".local";
  doc["ap_ip"] = wifiStatus.apIP;
  doc["ap_active"] = wifiStatus.apActive;
  doc["sta_connected"] = wifiStatus.staConnected;
  doc["sta_ip"] = wifiStatus.staIP;
  doc["sta_ssid"] = wifiStatus.staSsid;
  doc["signal_strength"] = WifiMgr.getSignalStrength();
  doc["connected_clients"] = wifiStatus.connectedClients;
  
  // Informações do sistema de arquivos
  doc["fs_total"] = Storage.getTotalSpace();
  doc["fs_used"] = Storage.getUsedSpace();
  doc["fs_free"] = Storage.getFreeSpace();
  
  // Versão do firmware
  doc["version"] = FIRMWARE_VERSION;
  
  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}

void WebServerManager::handleWiFiScan(AsyncWebServerRequest* request) {
  DEBUG_PRINTLN("[WebServer] GET /api/wifi/scan");
  String networks = WifiMgr.scanNetworks();
  request->send(200, "application/json", networks);
}

void WebServerManager::handleWiFiConnect(AsyncWebServerRequest* request, 
                                          uint8_t* data, 
                                          size_t len, 
                                          size_t index, 
                                          size_t total) {
  DEBUG_PRINTLN("[WebServer] POST /api/wifi/connect");
  
  StaticJsonDocument<256> doc;
  
  // Parse do JSON recebido
  StaticJsonDocument<256> inputDoc;
  DeserializationError error = deserializeJson(inputDoc, data, len);
  
  if (error) {
    doc["success"] = false;
    doc["message"] = "JSON inválido";
  } else {
    String ssid = inputDoc["ssid"] | "";
    String password = inputDoc["password"] | "";
    
    if (ssid.length() == 0) {
      doc["success"] = false;
      doc["message"] = "SSID não informado";
    } else {
      // Tenta conectar (isto vai bloquear por até 30 segundos)
      if (WifiMgr.connectToWiFi(ssid, password)) {
        doc["success"] = true;
        doc["message"] = "Conectado com sucesso!";
        doc["ip"] = WifiMgr.getMainIP();
      } else {
        doc["success"] = false;
        doc["message"] = "Falha ao conectar na rede";
      }
    }
  }
  
  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}

void WebServerManager::handleProxy(AsyncWebServerRequest* request) {
  DEBUG_PRINTLN("[WebServer] GET /proxy");
  
  if (!request->hasParam("url")) {
    request->send(400, "text/plain", "Parâmetro 'url' não informado");
    return;
  }
  
  String url = request->getParam("url")->value();
  DEBUG_PRINTF("[WebServer] Proxy para: %s\n", url.c_str());
  
  // Redireciona para a URL (mais eficiente que fazer proxy completo)
  // O ESP32 tem recursos limitados para fazer streaming de vídeo
  request->redirect(url);
}

void WebServerManager::handleDeletePlaylist(AsyncWebServerRequest* request) {
  DEBUG_PRINTLN("[WebServer] DELETE /playlist");
  
  StaticJsonDocument<128> doc;
  
  if (Storage.deleteFile(PLAYLIST_FILENAME)) {
    Playlist.clear();
    doc["success"] = true;
    doc["message"] = "Playlist apagada";
  } else {
    doc["success"] = false;
    doc["message"] = "Erro ao apagar playlist";
  }
  
  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}

void WebServerManager::handleNotFound(AsyncWebServerRequest* request) {
  DEBUG_PRINTF("[WebServer] 404: %s\n", request->url().c_str());
  request->send(404, "text/plain", "Recurso não encontrado");
}

// ═══════════════════════════════════════════════════════════════
// CONTEÚDO HTML
// ═══════════════════════════════════════════════════════════════

String WebServerManager::getHTML() {
  return R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 IPTV Server</title>
  <link rel="stylesheet" href="/style.css">
</head>
<body>
  <div class="container">
    <header>
      <h1>📺 ESP32 IPTV Server</h1>
      <p class="subtitle">Gerencie sua playlist IPTV</p>
    </header>
    
    <!-- Status do Sistema -->
    <section class="card" id="status-card">
      <h2>📊 Status do Sistema</h2>
      <div class="status-grid">
        <div class="status-item">
          <span class="label">IP:</span>
          <span id="status-ip">-</span>
        </div>
        <div class="status-item">
          <span class="label">Hostname:</span>
          <span id="status-hostname">-</span>
        </div>
        <div class="status-item">
          <span class="label">Uptime:</span>
          <span id="status-uptime">-</span>
        </div>
        <div class="status-item">
          <span class="label">RAM Livre:</span>
          <span id="status-ram">-</span>
        </div>
        <div class="status-item">
          <span class="label">Playlist:</span>
          <span id="status-file">-</span>
        </div>
        <div class="status-item">
          <span class="label">Canais:</span>
          <span id="status-channels">-</span>
        </div>
      </div>
    </section>
    
    <!-- Upload de Playlist -->
    <section class="card">
      <h2>📤 Upload de Playlist</h2>
      <form id="upload-form" enctype="multipart/form-data">
        <div class="file-input-wrapper">
          <input type="file" id="file-input" name="file" accept=".m3u,.m3u8" required>
          <label for="file-input" class="file-label">
            <span id="file-name">Escolher arquivo .m3u8</span>
          </label>
        </div>
        <button type="submit" class="btn btn-primary" id="upload-btn">
          <span>Upload</span>
        </button>
        <div id="upload-progress" class="progress-bar" style="display:none;">
          <div class="progress-fill"></div>
        </div>
        <div id="upload-message" class="message"></div>
      </form>
    </section>
    
    <!-- URL da Playlist -->
    <section class="card">
      <h2>🔗 URL da Playlist</h2>
      <div class="url-box">
        <input type="text" id="playlist-url" readonly>
        <button class="btn btn-secondary" onclick="copyURL()">Copiar</button>
      </div>
      <p class="hint">Use esta URL no VLC, Kodi ou qualquer player IPTV</p>
    </section>
    
    <!-- Lista de Canais -->
    <section class="card">
      <h2>📺 Canais (<span id="channel-count">0</span>)</h2>
      <div class="search-box">
        <input type="text" id="search-input" placeholder="Buscar canal...">
      </div>
      <div id="channel-list" class="channel-list">
        <p class="empty-message">Nenhuma playlist carregada</p>
      </div>
    </section>
    
    <!-- Configuração WiFi -->
    <section class="card">
      <h2>📶 Configuração WiFi</h2>
      <div class="wifi-status">
        <p>AP: <strong id="wifi-ap-ssid">-</strong> (IP: <span id="wifi-ap-ip">-</span>)</p>
        <p>Station: <strong id="wifi-sta-status">Desconectado</strong></p>
      </div>
      <button class="btn btn-secondary" onclick="scanWiFi()">Escanear Redes</button>
      <div id="wifi-networks" class="network-list" style="display:none;"></div>
      <div id="wifi-connect-form" style="display:none;">
        <input type="text" id="wifi-ssid" placeholder="SSID" readonly>
        <input type="password" id="wifi-password" placeholder="Senha">
        <button class="btn btn-primary" onclick="connectWiFi()">Conectar</button>
      </div>
    </section>
    
    <!-- Ações -->
    <section class="card">
      <h2>⚙️ Ações</h2>
      <div class="actions">
        <button class="btn btn-danger" onclick="deletePlaylist()">Apagar Playlist</button>
        <button class="btn btn-secondary" onclick="refreshStatus()">Atualizar Status</button>
      </div>
    </section>
    
    <footer>
      <p>ESP32 IPTV Server v<span id="version">1.0.0</span></p>
    </footer>
  </div>
  
  <script src="/script.js"></script>
</body>
</html>
)rawliteral";
}

// ═══════════════════════════════════════════════════════════════
// CONTEÚDO CSS
// ═══════════════════════════════════════════════════════════════

String WebServerManager::getCSS() {
  return R"rawliteral(
* {
  box-sizing: border-box;
  margin: 0;
  padding: 0;
}

body {
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
  background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
  min-height: 100vh;
  color: #e0e0e0;
  line-height: 1.6;
}

.container {
  max-width: 800px;
  margin: 0 auto;
  padding: 20px;
}

header {
  text-align: center;
  padding: 30px 0;
}

header h1 {
  font-size: 2rem;
  color: #00d4ff;
  margin-bottom: 5px;
}

.subtitle {
  color: #888;
  font-size: 0.9rem;
}

.card {
  background: rgba(255, 255, 255, 0.05);
  border-radius: 15px;
  padding: 20px;
  margin-bottom: 20px;
  border: 1px solid rgba(255, 255, 255, 0.1);
  backdrop-filter: blur(10px);
}

.card h2 {
  font-size: 1.2rem;
  margin-bottom: 15px;
  color: #00d4ff;
  border-bottom: 1px solid rgba(255, 255, 255, 0.1);
  padding-bottom: 10px;
}

.status-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
  gap: 15px;
}

.status-item {
  background: rgba(0, 0, 0, 0.2);
  padding: 12px;
  border-radius: 8px;
}

.status-item .label {
  display: block;
  font-size: 0.75rem;
  color: #888;
  text-transform: uppercase;
  margin-bottom: 3px;
}

.status-item span:last-child {
  font-size: 1rem;
  font-weight: 600;
  color: #00d4ff;
}

.file-input-wrapper {
  margin-bottom: 15px;
}

.file-input-wrapper input[type="file"] {
  display: none;
}

.file-label {
  display: block;
  padding: 20px;
  background: rgba(0, 212, 255, 0.1);
  border: 2px dashed rgba(0, 212, 255, 0.3);
  border-radius: 10px;
  text-align: center;
  cursor: pointer;
  transition: all 0.3s;
}

.file-label:hover {
  background: rgba(0, 212, 255, 0.2);
  border-color: rgba(0, 212, 255, 0.5);
}

.btn {
  padding: 12px 24px;
  border: none;
  border-radius: 8px;
  font-size: 1rem;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s;
  display: inline-flex;
  align-items: center;
  gap: 8px;
}

.btn-primary {
  background: linear-gradient(135deg, #00d4ff 0%, #0099cc 100%);
  color: #000;
}

.btn-primary:hover {
  transform: translateY(-2px);
  box-shadow: 0 5px 20px rgba(0, 212, 255, 0.4);
}

.btn-secondary {
  background: rgba(255, 255, 255, 0.1);
  color: #e0e0e0;
  border: 1px solid rgba(255, 255, 255, 0.2);
}

.btn-secondary:hover {
  background: rgba(255, 255, 255, 0.2);
}

.btn-danger {
  background: linear-gradient(135deg, #ff4757 0%, #cc0000 100%);
  color: #fff;
}

.btn-danger:hover {
  transform: translateY(-2px);
  box-shadow: 0 5px 20px rgba(255, 71, 87, 0.4);
}

.progress-bar {
  height: 4px;
  background: rgba(255, 255, 255, 0.1);
  border-radius: 2px;
  margin-top: 15px;
  overflow: hidden;
}

.progress-fill {
  height: 100%;
  background: linear-gradient(90deg, #00d4ff, #0099cc);
  width: 0%;
  transition: width 0.3s;
}

.message {
  margin-top: 15px;
  padding: 10px;
  border-radius: 8px;
  display: none;
}

.message.success {
  background: rgba(0, 255, 0, 0.1);
  color: #4caf50;
  display: block;
}

.message.error {
  background: rgba(255, 0, 0, 0.1);
  color: #f44336;
  display: block;
}

.url-box {
  display: flex;
  gap: 10px;
}

.url-box input {
  flex: 1;
  padding: 12px;
  background: rgba(0, 0, 0, 0.3);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 8px;
  color: #00d4ff;
  font-family: monospace;
}

.hint {
  font-size: 0.8rem;
  color: #666;
  margin-top: 10px;
}

.search-box {
  margin-bottom: 15px;
}

.search-box input {
  width: 100%;
  padding: 12px;
  background: rgba(0, 0, 0, 0.3);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 8px;
  color: #e0e0e0;
}

.search-box input:focus {
  outline: none;
  border-color: #00d4ff;
}

.channel-list {
  max-height: 400px;
  overflow-y: auto;
}

.channel-item {
  display: flex;
  align-items: center;
  padding: 12px;
  background: rgba(0, 0, 0, 0.2);
  border-radius: 8px;
  margin-bottom: 8px;
  transition: all 0.3s;
}

.channel-item:hover {
  background: rgba(0, 212, 255, 0.1);
}

.channel-logo {
  width: 40px;
  height: 40px;
  border-radius: 8px;
  margin-right: 12px;
  background: rgba(255, 255, 255, 0.1);
  object-fit: cover;
}

.channel-info {
  flex: 1;
}

.channel-name {
  font-weight: 600;
  color: #e0e0e0;
}

.channel-group {
  font-size: 0.75rem;
  color: #666;
}

.empty-message {
  text-align: center;
  color: #666;
  padding: 30px;
}

.network-list {
  margin-top: 15px;
}

.network-item {
  padding: 12px;
  background: rgba(0, 0, 0, 0.2);
  border-radius: 8px;
  margin-bottom: 8px;
  cursor: pointer;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.network-item:hover {
  background: rgba(0, 212, 255, 0.1);
}

.wifi-status p {
  margin-bottom: 5px;
}

#wifi-connect-form {
  margin-top: 15px;
  display: flex;
  flex-direction: column;
  gap: 10px;
}

#wifi-connect-form input {
  padding: 12px;
  background: rgba(0, 0, 0, 0.3);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 8px;
  color: #e0e0e0;
}

.actions {
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
}

footer {
  text-align: center;
  padding: 20px;
  color: #666;
  font-size: 0.8rem;
}

@media (max-width: 600px) {
  .container {
    padding: 10px;
  }
  
  header h1 {
    font-size: 1.5rem;
  }
  
  .url-box {
    flex-direction: column;
  }
  
  .actions {
    flex-direction: column;
  }
  
  .btn {
    width: 100%;
    justify-content: center;
  }
}

::-webkit-scrollbar {
  width: 8px;
}

::-webkit-scrollbar-track {
  background: rgba(0, 0, 0, 0.2);
}

::-webkit-scrollbar-thumb {
  background: rgba(0, 212, 255, 0.3);
  border-radius: 4px;
}

::-webkit-scrollbar-thumb:hover {
  background: rgba(0, 212, 255, 0.5);
}
)rawliteral";
}

// ═══════════════════════════════════════════════════════════════
// CONTEÚDO JAVASCRIPT
// ═══════════════════════════════════════════════════════════════

String WebServerManager::getJS() {
  return R"rawliteral(
// Atualiza a URL da playlist
function updatePlaylistURL() {
  const url = window.location.origin + '/playlist.m3u8';
  document.getElementById('playlist-url').value = url;
}

// Copia URL para clipboard
function copyURL() {
  const input = document.getElementById('playlist-url');
  input.select();
  document.execCommand('copy');
  alert('URL copiada!');
}

// Atualiza status do sistema
async function refreshStatus() {
  try {
    const response = await fetch('/api/status');
    const data = await response.json();
    
    document.getElementById('status-ip').textContent = data.ip;
    document.getElementById('status-hostname').textContent = data.hostname + ':8080';
    document.getElementById('status-uptime').textContent = formatUptime(data.uptime);
    document.getElementById('status-ram').textContent = formatBytes(data.free_ram);
    document.getElementById('status-file').textContent = data.file_exists ? formatBytes(data.file_size) : 'Não existe';
    document.getElementById('status-channels').textContent = data.channel_count;
    document.getElementById('version').textContent = data.version;
    
    // WiFi status
    document.getElementById('wifi-ap-ip').textContent = data.ap_ip;
    document.getElementById('wifi-sta-status').textContent = 
      data.sta_connected ? `Conectado em ${data.sta_ssid} (${data.sta_ip})` : 'Desconectado';
  } catch (error) {
    console.error('Erro ao atualizar status:', error);
  }
}

// Formata bytes para exibição
function formatBytes(bytes) {
  if (bytes === 0) return '0 B';
  const k = 1024;
  const sizes = ['B', 'KB', 'MB', 'GB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

// Formata uptime
function formatUptime(seconds) {
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  const s = seconds % 60;
  return `${h}h ${m}m ${s}s`;
}

// Carrega lista de canais
async function loadChannels() {
  try {
    const response = await fetch('/list.json');
    const data = await response.json();
    
    document.getElementById('channel-count').textContent = data.total || 0;
    
    const list = document.getElementById('channel-list');
    
    if (!data.channels || data.channels.length === 0) {
      list.innerHTML = '<p class="empty-message">Nenhuma playlist carregada</p>';
      return;
    }
    
    list.innerHTML = '';
    data.channels.forEach((channel, index) => {
      const div = document.createElement('div');
      div.className = 'channel-item';
      div.innerHTML = `
        <img src="${channel.logo || 'data:image/svg+xml,%3Csvg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\"%3E%3Cpath fill=\"%23666\" d=\"M21 3H3c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h18c1.1 0 2-.9 2-2V5c0-1.1-.9-2-2-2zm0 16H3V5h18v14z\"%3E%3C/path%3E%3C/svg%3E'}" 
             class="channel-logo" 
             onerror="this.src='data:image/svg+xml,%3Csvg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\"%3E%3Cpath fill=\"%23666\" d=\"M21 3H3c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h18c1.1 0 2-.9 2-2V5c0-1.1-.9-2-2-2zm0 16H3V5h18v14z\"%3E%3C/path%3E%3C/svg%3E'">
        <div class="channel-info">
          <div class="channel-name">${channel.name}</div>
          <div class="channel-group">${channel.group || 'Sem grupo'}</div>
        </div>
      `;
      div.onclick = () => window.open(channel.url, '_blank');
      list.appendChild(div);
    });
  } catch (error) {
    console.error('Erro ao carregar canais:', error);
  }
}

// Busca canais
document.getElementById('search-input').addEventListener('input', function(e) {
  const query = e.target.value.toLowerCase();
  const items = document.querySelectorAll('.channel-item');
  
  items.forEach(item => {
    const name = item.querySelector('.channel-name').textContent.toLowerCase();
    item.style.display = name.includes(query) ? 'flex' : 'none';
  });
});

// Upload de arquivo
document.getElementById('file-input').addEventListener('change', function(e) {
  const fileName = e.target.files[0]?.name || 'Escolher arquivo .m3u8';
  document.getElementById('file-name').textContent = fileName;
});

document.getElementById('upload-form').addEventListener('submit', async function(e) {
  e.preventDefault();
  
  const fileInput = document.getElementById('file-input');
  const file = fileInput.files[0];
  
  if (!file) {
    showMessage('Selecione um arquivo', false);
    return;
  }
  
  const formData = new FormData();
  formData.append('file', file);
  
  const progressBar = document.getElementById('upload-progress');
  const progressFill = progressBar.querySelector('.progress-fill');
  const uploadBtn = document.getElementById('upload-btn');
  
  progressBar.style.display = 'block';
  uploadBtn.disabled = true;
  
  try {
    const response = await fetch('/upload', {
      method: 'POST',
      body: formData
    });
    
    const data = await response.json();
    showMessage(data.message, data.success);
    
    if (data.success) {
      loadChannels();
      refreshStatus();
    }
  } catch (error) {
    showMessage('Erro no upload: ' + error.message, false);
  } finally {
    progressBar.style.display = 'none';
    uploadBtn.disabled = false;
    progressFill.style.width = '0%';
  }
});

function showMessage(text, success) {
  const msg = document.getElementById('upload-message');
  msg.textContent = text;
  msg.className = 'message ' + (success ? 'success' : 'error');
}

// Escaneia redes WiFi
async function scanWiFi() {
  const list = document.getElementById('wifi-networks');
  list.style.display = 'block';
  list.innerHTML = '<p>Escaneando...</p>';
  
  try {
    const response = await fetch('/api/wifi/scan');
    const data = await response.json();
    
    if (data.networks.length === 0) {
      list.innerHTML = '<p>Nenhuma rede encontrada</p>';
      return;
    }
    
    list.innerHTML = '';
    data.networks.forEach(network => {
      const div = document.createElement('div');
      div.className = 'network-item';
      div.innerHTML = `
        <span>${network.ssid}</span>
        <span>${network.rssi} dBm (${network.encryption})</span>
      `;
      div.onclick = () => selectNetwork(network.ssid);
      list.appendChild(div);
    });
  } catch (error) {
    list.innerHTML = '<p>Erro ao escanear</p>';
  }
}

function selectNetwork(ssid) {
  document.getElementById('wifi-ssid').value = ssid;
  document.getElementById('wifi-connect-form').style.display = 'flex';
}

async function connectWiFi() {
  const ssid = document.getElementById('wifi-ssid').value;
  const password = document.getElementById('wifi-password').value;
  
  try {
    const response = await fetch('/api/wifi/connect', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ ssid, password })
    });
    
    const data = await response.json();
    alert(data.message);
    
    if (data.success) {
      refreshStatus();
    }
  } catch (error) {
    alert('Erro ao conectar: ' + error.message);
  }
}

// Apaga playlist
async function deletePlaylist() {
  if (!confirm('Tem certeza que deseja apagar a playlist?')) return;
  
  try {
    const response = await fetch('/playlist', { method: 'DELETE' });
    const data = await response.json();
    alert(data.message);
    loadChannels();
    refreshStatus();
  } catch (error) {
    alert('Erro ao apagar: ' + error.message);
  }
}

// Inicialização
document.addEventListener('DOMContentLoaded', function() {
  updatePlaylistURL();
  refreshStatus();
  loadChannels();
  
  // Atualiza status a cada 5 segundos
  setInterval(refreshStatus, 5000);
});
)rawliteral";
}

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
  
  // API Playlist (proxy para R2 - resolve CORS)
  _server->on("/api/playlist", HTTP_GET, handlePlaylistProxy);
  
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
  
  #if USE_REMOTE_PLAYLIST
    // Modo de playlist remota - redireciona para Cloudflare R2
    DEBUG_PRINTLN("[WebServer] Redirecionando para playlist remota...");
    DEBUG_PRINTF("[WebServer] URL: %s\n", REMOTE_PLAYLIST_URL);
    
    // Usa redirect 302 (temporário) para que o cliente busque diretamente do R2
    request->redirect(REMOTE_PLAYLIST_URL);
    return;
  #else
    // Modo de playlist local
    if (!Storage.fileExists(PLAYLIST_FILENAME)) {
      DEBUG_PRINTLN("[WebServer] Playlist não encontrada!");
      request->send(404, "text/plain", "Playlist não encontrada");
      return;
    }
    
    // Lê o conteúdo do arquivo e envia
    String content = Storage.readFile(PLAYLIST_FILENAME);
    if (content.length() == 0) {
      DEBUG_PRINTLN("[WebServer] Playlist vazia!");
      request->send(404, "text/plain", "Playlist vazia");
      return;
    }
    
    DEBUG_PRINTF("[WebServer] Enviando playlist: %d bytes\n", content.length());
    request->send(200, "application/vnd.apple.mpegurl", content);
  #endif
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
  
  StaticJsonDocument<768> doc;
  
  // Uptime em segundos
  doc["uptime"] = millis() / 1000;
  
  // RAM livre
  doc["free_ram"] = ESP.getFreeHeap();
  doc["total_ram"] = ESP.getHeapSize();
  
  // Informações da playlist
  #if USE_REMOTE_PLAYLIST
    doc["playlist_mode"] = "remote";
    doc["playlist_url"] = REMOTE_PLAYLIST_URL;
    doc["file_size"] = 0;
    doc["file_exists"] = true;
    doc["channel_count"] = -1; // -1 indica playlist remota (não parseada localmente)
  #else
    doc["playlist_mode"] = "local";
    doc["playlist_url"] = "";
    doc["file_size"] = Storage.getFileSize(PLAYLIST_FILENAME);
    doc["file_exists"] = Storage.fileExists(PLAYLIST_FILENAME);
    doc["channel_count"] = Playlist.getChannelCount();
  #endif
  
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

// Handler para servir a playlist (local ou redireciona para remota)
void WebServerManager::handlePlaylistProxy(AsyncWebServerRequest* request) {
  DEBUG_PRINTLN("[WebServer] GET /api/playlist");
  
  // Primeiro tenta playlist local
  if (Storage.fileExists(PLAYLIST_FILENAME)) {
    String content = Storage.readFile(PLAYLIST_FILENAME);
    if (content.length() > 0) {
      DEBUG_PRINTF("[WebServer] Servindo playlist local: %d bytes\n", content.length());
      AsyncWebServerResponse* response = request->beginResponse(200, "application/x-mpegurl", content);
      response->addHeader("Access-Control-Allow-Origin", "*");
      response->addHeader("Cache-Control", "max-age=60");
      request->send(response);
      return;
    }
  }
  
  // Fallback: retorna URL para o cliente baixar diretamente
  // (necessário configurar CORS no R2)
  StaticJsonDocument<512> doc;
  doc["error"] = "Nenhuma playlist local encontrada";
  doc["remote_url"] = REMOTE_PLAYLIST_URL;
  doc["hint"] = "Configure CORS no Cloudflare R2 ou faça upload de uma playlist local";
  
  String response;
  serializeJson(doc, response);
  request->send(404, "application/json", response);
}

void WebServerManager::handleNotFound(AsyncWebServerRequest* request) {
  DEBUG_PRINTF("[WebServer] 404: %s\n", request->url().c_str());
  request->send(404, "text/plain", "Recurso não encontrado");
}

// ═══════════════════════════════════════════════════════════════
// CONTEÚDO HTML
// ═══════════════════════════════════════════════════════════════

String WebServerManager::getHTML() {
  String remoteUrl = REMOTE_PLAYLIST_URL;
  return R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>ESP32 IPTV Player</title>
  <link rel="stylesheet" href="/style.css">
  <script src="https://cdn.jsdelivr.net/npm/hls.js@latest"></script>
</head>
<body>
  <div id="app">
    <!-- Player de Vídeo (Modal) -->
    <div id="player-modal" class="modal hidden">
      <div class="modal-content">
        <div class="player-header">
          <span id="player-title">Carregando...</span>
          <button class="close-btn" onclick="closePlayer()">&times;</button>
        </div>
        <div class="video-container">
          <video id="video-player" controls autoplay playsinline></video>
          <div id="player-loading" class="loading-overlay">
            <div class="spinner"></div>
            <span>Carregando stream...</span>
          </div>
          <div id="player-error" class="error-overlay hidden">
            <span>⚠️ Erro ao carregar stream</span>
            <button onclick="retryStream()">Tentar novamente</button>
          </div>
        </div>
      </div>
    </div>

    <!-- Header -->
    <header>
      <div class="header-content">
        <h1>📺 ESP32 IPTV</h1>
        <div class="header-info">
          <span id="header-status" class="status-badge online">● Online</span>
          <span id="channel-total">0 canais</span>
        </div>
      </div>
    </header>

    <!-- Barra de Pesquisa e Filtros -->
    <div class="controls">
      <div class="search-wrapper">
        <input type="text" id="search" placeholder="🔍 Buscar canal..." autocomplete="off">
        <button id="clear-search" class="hidden" onclick="clearSearch()">✕</button>
      </div>
      <div class="filter-wrapper">
        <select id="category-filter">
          <option value="">Todas as categorias</option>
        </select>
        <select id="quality-filter">
          <option value="">Qualidade</option>
          <option value="FHD">FHD</option>
          <option value="HD">HD</option>
          <option value="SD">SD</option>
        </select>
      </div>
    </div>

    <!-- Loading inicial -->
    <div id="loading-screen" class="loading-screen">
      <div class="spinner large"></div>
      <p>Carregando playlist...</p>
      <p class="loading-info" id="loading-info"></p>
    </div>

    <!-- Grid de Canais -->
    <main id="channel-grid" class="channel-grid hidden"></main>

    <!-- Mensagem de erro -->
    <div id="error-screen" class="error-screen hidden">
      <span class="error-icon">📡</span>
      <h2>Erro ao carregar playlist</h2>
      <p id="error-message"></p>
      <button onclick="loadPlaylist()">Tentar novamente</button>
    </div>

    <!-- Footer com status -->
    <footer>
      <div class="footer-content">
        <span>ESP32 IPTV Server v1.0</span>
        <span id="footer-ip"></span>
      </div>
    </footer>
  </div>

  <script>
    const PLAYLIST_URL = ')rawliteral" + remoteUrl + R"rawliteral(';
  </script>
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
:root {
  --primary: #00d4ff;
  --primary-dark: #0099cc;
  --bg-dark: #0a0a1a;
  --bg-card: rgba(255,255,255,0.05);
  --text: #e0e0e0;
  --text-muted: #888;
  --success: #4caf50;
  --error: #ff4757;
}

* { box-sizing: border-box; margin: 0; padding: 0; }

body {
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
  background: linear-gradient(135deg, #0a0a1a 0%, #1a1a3a 100%);
  min-height: 100vh;
  color: var(--text);
  overflow-x: hidden;
}

.hidden { display: none !important; }

/* Header */
header {
  background: rgba(0,0,0,0.5);
  backdrop-filter: blur(10px);
  padding: 15px 20px;
  position: sticky;
  top: 0;
  z-index: 100;
  border-bottom: 1px solid rgba(255,255,255,0.1);
}

.header-content {
  max-width: 1400px;
  margin: 0 auto;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

header h1 {
  font-size: 1.5rem;
  color: var(--primary);
}

.header-info {
  display: flex;
  gap: 15px;
  align-items: center;
  font-size: 0.9rem;
  color: var(--text-muted);
}

.status-badge {
  padding: 4px 10px;
  border-radius: 20px;
  font-size: 0.8rem;
}

.status-badge.online {
  background: rgba(76,175,80,0.2);
  color: var(--success);
}

/* Controls */
.controls {
  max-width: 1400px;
  margin: 20px auto;
  padding: 0 20px;
  display: flex;
  gap: 15px;
  flex-wrap: wrap;
}

.search-wrapper {
  flex: 1;
  min-width: 250px;
  position: relative;
}

.search-wrapper input {
  width: 100%;
  padding: 14px 45px 14px 18px;
  background: var(--bg-card);
  border: 1px solid rgba(255,255,255,0.1);
  border-radius: 12px;
  color: var(--text);
  font-size: 1rem;
}

.search-wrapper input:focus {
  outline: none;
  border-color: var(--primary);
  box-shadow: 0 0 15px rgba(0,212,255,0.2);
}

#clear-search {
  position: absolute;
  right: 12px;
  top: 50%;
  transform: translateY(-50%);
  background: none;
  border: none;
  color: var(--text-muted);
  font-size: 1.2rem;
  cursor: pointer;
  padding: 5px;
}

.filter-wrapper {
  display: flex;
  gap: 10px;
}

.filter-wrapper select {
  padding: 14px 18px;
  background: var(--bg-card);
  border: 1px solid rgba(255,255,255,0.1);
  border-radius: 12px;
  color: var(--text);
  font-size: 0.9rem;
  cursor: pointer;
  min-width: 150px;
}

.filter-wrapper select:focus {
  outline: none;
  border-color: var(--primary);
}

/* Channel Grid */
.channel-grid {
  max-width: 1400px;
  margin: 0 auto;
  padding: 0 20px 100px;
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
  gap: 15px;
}

.channel-card {
  background: var(--bg-card);
  border: 1px solid rgba(255,255,255,0.08);
  border-radius: 12px;
  padding: 15px;
  display: flex;
  align-items: center;
  gap: 12px;
  cursor: pointer;
  transition: all 0.2s;
}

.channel-card:hover {
  background: rgba(0,212,255,0.1);
  border-color: rgba(0,212,255,0.3);
  transform: translateY(-2px);
}

.channel-card:active {
  transform: scale(0.98);
}

.channel-logo {
  width: 50px;
  height: 50px;
  border-radius: 10px;
  background: rgba(0,0,0,0.3);
  object-fit: contain;
  flex-shrink: 0;
}

.channel-info {
  flex: 1;
  min-width: 0;
}

.channel-name {
  font-weight: 600;
  font-size: 0.95rem;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.channel-meta {
  display: flex;
  gap: 8px;
  margin-top: 4px;
  flex-wrap: wrap;
}

.channel-group {
  font-size: 0.75rem;
  color: var(--text-muted);
  background: rgba(255,255,255,0.05);
  padding: 2px 8px;
  border-radius: 4px;
}

.channel-quality {
  font-size: 0.7rem;
  padding: 2px 6px;
  border-radius: 4px;
  font-weight: 600;
}

.quality-fhd { background: #4caf50; color: #000; }
.quality-hd { background: #2196f3; color: #fff; }
.quality-sd { background: #ff9800; color: #000; }

.play-icon {
  width: 36px;
  height: 36px;
  background: var(--primary);
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  color: #000;
  font-size: 1rem;
  flex-shrink: 0;
  opacity: 0;
  transition: opacity 0.2s;
}

.channel-card:hover .play-icon {
  opacity: 1;
}

/* Loading Screen */
.loading-screen {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  min-height: 60vh;
  gap: 20px;
}

.spinner {
  width: 40px;
  height: 40px;
  border: 3px solid rgba(0,212,255,0.2);
  border-top-color: var(--primary);
  border-radius: 50%;
  animation: spin 1s linear infinite;
}

.spinner.large {
  width: 60px;
  height: 60px;
  border-width: 4px;
}

@keyframes spin {
  to { transform: rotate(360deg); }
}

.loading-info {
  font-size: 0.85rem;
  color: var(--text-muted);
}

/* Error Screen */
.error-screen {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  min-height: 60vh;
  gap: 15px;
  text-align: center;
  padding: 20px;
}

.error-icon {
  font-size: 4rem;
}

.error-screen button {
  margin-top: 10px;
  padding: 12px 30px;
  background: var(--primary);
  border: none;
  border-radius: 8px;
  color: #000;
  font-weight: 600;
  cursor: pointer;
}

/* Video Player Modal */
.modal {
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: rgba(0,0,0,0.95);
  z-index: 1000;
  display: flex;
  align-items: center;
  justify-content: center;
}

.modal-content {
  width: 100%;
  max-width: 1200px;
  max-height: 100vh;
  display: flex;
  flex-direction: column;
}

.player-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 15px 20px;
  background: rgba(0,0,0,0.5);
}

#player-title {
  font-size: 1.1rem;
  font-weight: 600;
}

.close-btn {
  background: none;
  border: none;
  color: var(--text);
  font-size: 2rem;
  cursor: pointer;
  padding: 0 10px;
  line-height: 1;
}

.close-btn:hover {
  color: var(--error);
}

.video-container {
  position: relative;
  width: 100%;
  background: #000;
}

#video-player {
  width: 100%;
  max-height: calc(100vh - 60px);
  background: #000;
}

.loading-overlay, .error-overlay {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background: rgba(0,0,0,0.8);
  gap: 15px;
}

.error-overlay {
  color: var(--error);
}

.error-overlay button {
  padding: 10px 25px;
  background: var(--primary);
  border: none;
  border-radius: 6px;
  color: #000;
  font-weight: 600;
  cursor: pointer;
}

/* Footer */
footer {
  position: fixed;
  bottom: 0;
  left: 0;
  width: 100%;
  background: rgba(0,0,0,0.8);
  backdrop-filter: blur(10px);
  padding: 12px 20px;
  border-top: 1px solid rgba(255,255,255,0.1);
}

.footer-content {
  max-width: 1400px;
  margin: 0 auto;
  display: flex;
  justify-content: space-between;
  font-size: 0.85rem;
  color: var(--text-muted);
}

/* Responsive */
@media (max-width: 768px) {
  header h1 { font-size: 1.2rem; }
  .controls { padding: 0 15px; }
  .channel-grid { padding: 0 15px 100px; grid-template-columns: 1fr; }
  .filter-wrapper { width: 100%; }
  .filter-wrapper select { flex: 1; min-width: auto; }
  .modal-content { max-width: 100%; }
  .player-header { padding: 10px 15px; }
  #player-title { font-size: 0.95rem; }
}

/* Scrollbar */
::-webkit-scrollbar { width: 8px; }
::-webkit-scrollbar-track { background: rgba(0,0,0,0.2); }
::-webkit-scrollbar-thumb { background: rgba(0,212,255,0.3); border-radius: 4px; }
::-webkit-scrollbar-thumb:hover { background: rgba(0,212,255,0.5); }
)rawliteral";
}

// ═══════════════════════════════════════════════════════════════
// CONTEÚDO JAVASCRIPT
// ═══════════════════════════════════════════════════════════════

String WebServerManager::getJS() {
  return R"rawliteral(
// Estado global
let allChannels = [];
let filteredChannels = [];
let currentChannel = null;
let hls = null;

// Elementos DOM
const elements = {
  loadingScreen: document.getElementById('loading-screen'),
  loadingInfo: document.getElementById('loading-info'),
  errorScreen: document.getElementById('error-screen'),
  errorMessage: document.getElementById('error-message'),
  channelGrid: document.getElementById('channel-grid'),
  channelTotal: document.getElementById('channel-total'),
  searchInput: document.getElementById('search'),
  clearSearch: document.getElementById('clear-search'),
  categoryFilter: document.getElementById('category-filter'),
  qualityFilter: document.getElementById('quality-filter'),
  playerModal: document.getElementById('player-modal'),
  playerTitle: document.getElementById('player-title'),
  videoPlayer: document.getElementById('video-player'),
  playerLoading: document.getElementById('player-loading'),
  playerError: document.getElementById('player-error'),
  footerIp: document.getElementById('footer-ip')
};

// Inicialização
document.addEventListener('DOMContentLoaded', async () => {
  await loadStatus();
  await loadPlaylist();
  setupEventListeners();
});

// Carrega status do ESP32
async function loadStatus() {
  try {
    const res = await fetch('/api/status');
    const data = await res.json();
    elements.footerIp.textContent = `IP: ${data.ip}`;
  } catch (e) {
    elements.footerIp.textContent = 'ESP32 IPTV';
  }
}

// Carrega playlist via proxy do ESP32 (resolve CORS)
async function loadPlaylist() {
  elements.loadingInfo.textContent = 'Baixando playlist do servidor...';
  
  try {
    // Usa proxy local para evitar CORS
    const res = await fetch('/api/playlist');
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    
    elements.loadingInfo.textContent = 'Processando canais...';
    const text = await res.text();
    
    allChannels = parseM3U(text);
    filteredChannels = [...allChannels];
    
    populateFilters();
    renderChannels();
    
    elements.channelTotal.textContent = `${allChannels.length} canais`;
    elements.loadingScreen.classList.add('hidden');
    elements.channelGrid.classList.remove('hidden');
    
  } catch (error) {
    console.error('Erro ao carregar playlist:', error);
    elements.loadingScreen.classList.add('hidden');
    elements.errorScreen.classList.remove('hidden');
    elements.errorMessage.textContent = error.message;
  }
}

// Parser M3U8
function parseM3U(text) {
  const lines = text.split('\n');
  const channels = [];
  
  for (let i = 0; i < lines.length; i++) {
    const line = lines[i].trim();
    
    if (line.startsWith('#EXTINF')) {
      const channel = {
        name: '',
        url: '',
        logo: '',
        group: 'Outros',
        quality: ''
      };
      
      // Extrair nome (após última vírgula)
      const nameMatch = line.match(/,(.+)$/);
      if (nameMatch) channel.name = nameMatch[1].trim();
      
      // Extrair grupo
      const groupMatch = line.match(/group-title="([^"]+)"/i);
      if (groupMatch) channel.group = groupMatch[1];
      
      // Extrair logo
      const logoMatch = line.match(/tvg-logo="([^"]+)"/i);
      if (logoMatch) channel.logo = logoMatch[1];
      
      // Detectar qualidade
      if (channel.name.includes('FHD') || channel.name.includes('4K')) {
        channel.quality = 'FHD';
      } else if (channel.name.includes('HD') && !channel.name.includes('SD')) {
        channel.quality = 'HD';
      } else if (channel.name.includes('SD')) {
        channel.quality = 'SD';
      }
      
      // Próxima linha é a URL
      if (i + 1 < lines.length) {
        const url = lines[i + 1].trim();
        if (url && !url.startsWith('#')) {
          channel.url = url;
          channels.push(channel);
          i++;
        }
      }
    }
  }
  
  return channels;
}

// Popula filtros de categoria
function populateFilters() {
  const categories = [...new Set(allChannels.map(c => c.group))].sort();
  
  elements.categoryFilter.innerHTML = '<option value="">Todas as categorias</option>';
  categories.forEach(cat => {
    const option = document.createElement('option');
    option.value = cat;
    option.textContent = cat;
    elements.categoryFilter.appendChild(option);
  });
}

// Renderiza grid de canais
function renderChannels() {
  elements.channelGrid.innerHTML = '';
  
  if (filteredChannels.length === 0) {
    elements.channelGrid.innerHTML = '<div style="grid-column:1/-1;text-align:center;padding:50px;color:#888;">Nenhum canal encontrado</div>';
    return;
  }
  
  filteredChannels.forEach((channel, index) => {
    const card = document.createElement('div');
    card.className = 'channel-card';
    card.onclick = () => playChannel(channel);
    
    let qualityBadge = '';
    if (channel.quality) {
      const qClass = channel.quality === 'FHD' ? 'quality-fhd' : 
                     channel.quality === 'HD' ? 'quality-hd' : 'quality-sd';
      qualityBadge = `<span class="channel-quality ${qClass}">${channel.quality}</span>`;
    }
    
    card.innerHTML = `
      <img class="channel-logo" 
           src="${channel.logo || 'data:image/svg+xml,%3Csvg xmlns=%22http://www.w3.org/2000/svg%22 viewBox=%220 0 24 24%22%3E%3Cpath fill=%22%23444%22 d=%22M21 3H3c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h18c1.1 0 2-.9 2-2V5c0-1.1-.9-2-2-2zm0 16H3V5h18v14z%22/%3E%3C/svg%3E'}"
           onerror="this.src='data:image/svg+xml,%3Csvg xmlns=%22http://www.w3.org/2000/svg%22 viewBox=%220 0 24 24%22%3E%3Cpath fill=%22%23444%22 d=%22M21 3H3c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h18c1.1 0 2-.9 2-2V5c0-1.1-.9-2-2-2zm0 16H3V5h18v14z%22/%3E%3C/svg%3E'">
      <div class="channel-info">
        <div class="channel-name">${channel.name}</div>
        <div class="channel-meta">
          <span class="channel-group">${channel.group}</span>
          ${qualityBadge}
        </div>
      </div>
      <div class="play-icon">▶</div>
    `;
    
    elements.channelGrid.appendChild(card);
  });
}

// Filtra canais
function filterChannels() {
  const search = elements.searchInput.value.toLowerCase();
  const category = elements.categoryFilter.value;
  const quality = elements.qualityFilter.value;
  
  filteredChannels = allChannels.filter(channel => {
    const matchSearch = !search || 
                        channel.name.toLowerCase().includes(search) ||
                        channel.group.toLowerCase().includes(search);
    const matchCategory = !category || channel.group === category;
    const matchQuality = !quality || channel.quality === quality;
    
    return matchSearch && matchCategory && matchQuality;
  });
  
  elements.clearSearch.classList.toggle('hidden', !search);
  renderChannels();
}

function clearSearch() {
  elements.searchInput.value = '';
  filterChannels();
}

// Player de vídeo
function playChannel(channel) {
  currentChannel = channel;
  elements.playerTitle.textContent = channel.name;
  elements.playerModal.classList.remove('hidden');
  elements.playerLoading.classList.remove('hidden');
  elements.playerError.classList.add('hidden');
  
  document.body.style.overflow = 'hidden';
  
  // Limpa player anterior
  if (hls) {
    hls.destroy();
    hls = null;
  }
  
  const video = elements.videoPlayer;
  video.src = '';
  
  // Verifica se é HLS
  const isHLS = channel.url.includes('.m3u8') || channel.url.includes('/live/');
  
  if (isHLS && Hls.isSupported()) {
    hls = new Hls({
      maxBufferLength: 30,
      maxMaxBufferLength: 60,
      startLevel: -1,
      capLevelToPlayerSize: true
    });
    
    hls.loadSource(channel.url);
    hls.attachMedia(video);
    
    hls.on(Hls.Events.MANIFEST_PARSED, () => {
      elements.playerLoading.classList.add('hidden');
      video.play().catch(e => console.log('Autoplay blocked'));
    });
    
    hls.on(Hls.Events.ERROR, (event, data) => {
      if (data.fatal) {
        elements.playerLoading.classList.add('hidden');
        elements.playerError.classList.remove('hidden');
      }
    });
    
  } else if (video.canPlayType('application/vnd.apple.mpegurl')) {
    // Safari nativo HLS
    video.src = channel.url;
    video.addEventListener('loadedmetadata', () => {
      elements.playerLoading.classList.add('hidden');
      video.play().catch(e => console.log('Autoplay blocked'));
    }, { once: true });
    
    video.addEventListener('error', () => {
      elements.playerLoading.classList.add('hidden');
      elements.playerError.classList.remove('hidden');
    }, { once: true });
    
  } else {
    // Stream direto (MP4, etc)
    video.src = channel.url;
    video.addEventListener('canplay', () => {
      elements.playerLoading.classList.add('hidden');
      video.play().catch(e => console.log('Autoplay blocked'));
    }, { once: true });
    
    video.addEventListener('error', () => {
      elements.playerLoading.classList.add('hidden');
      elements.playerError.classList.remove('hidden');
    }, { once: true });
  }
}

function closePlayer() {
  elements.playerModal.classList.add('hidden');
  document.body.style.overflow = '';
  
  if (hls) {
    hls.destroy();
    hls = null;
  }
  
  elements.videoPlayer.pause();
  elements.videoPlayer.src = '';
  currentChannel = null;
}

function retryStream() {
  if (currentChannel) {
    playChannel(currentChannel);
  }
}

// Event Listeners
function setupEventListeners() {
  elements.searchInput.addEventListener('input', filterChannels);
  elements.categoryFilter.addEventListener('change', filterChannels);
  elements.qualityFilter.addEventListener('change', filterChannels);
  
  // Fechar player com ESC
  document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && !elements.playerModal.classList.contains('hidden')) {
      closePlayer();
    }
  });
  
  // Fechar player clicando fora
  elements.playerModal.addEventListener('click', (e) => {
    if (e.target === elements.playerModal) {
      closePlayer();
    }
  });
})rawliteral";
}
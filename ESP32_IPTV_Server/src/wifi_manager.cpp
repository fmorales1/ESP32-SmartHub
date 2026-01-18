/*
 * ═══════════════════════════════════════════════════════════════
 * ESP32 IPTV Server - Gerenciador WiFi (Implementação)
 * ═══════════════════════════════════════════════════════════════
 */

#include "wifi_manager.h"
#include "storage_manager.h"

// Instância global
WiFiManager WifiMgr;

// ═══════════════════════════════════════════════════════════════
// CONSTRUTOR
// ═══════════════════════════════════════════════════════════════

WiFiManager::WiFiManager() {
  _apActive = false;
  _staConnected = false;
  _lastReconnectAttempt = 0;
  _reconnectAttempts = 0;
}

// ═══════════════════════════════════════════════════════════════
// INICIALIZAÇÃO PRINCIPAL
// ═══════════════════════════════════════════════════════════════

bool WiFiManager::begin() {
  DEBUG_PRINTLN("[WiFi] Iniciando gerenciador WiFi...");
  
  // Registra callback de eventos WiFi
  WiFi.onEvent(onWiFiEvent);
  
  // Carrega configuração salva
  loadConfig();
  
  // Define modo AP+STA (ambos simultaneamente)
  WiFi.mode(WIFI_AP_STA);
  
  // Configura hostname
  WiFi.setHostname(MDNS_HOSTNAME);
  
  // Inicia Access Point
  if (!startAP()) {
    DEBUG_PRINTLN("[WiFi] ERRO: Falha ao iniciar Access Point!");
    // Continua mesmo assim, pode funcionar só como STA
  }
  
  // Se tem credenciais de Station configuradas, tenta conectar
  if (_config.stationEnabled && _config.staSsid.length() > 0) {
    DEBUG_PRINTF("[WiFi] Tentando conectar em: %s\n", _config.staSsid.c_str());
    startStation();
  }
  
  // Configura mDNS
  setupMDNS();
  
  return _apActive;
}

// ═══════════════════════════════════════════════════════════════
// ACCESS POINT
// ═══════════════════════════════════════════════════════════════

bool WiFiManager::startAP() {
  DEBUG_PRINTLN("[WiFi] Iniciando Access Point...");
  
  // Configura o AP
  WiFi.softAP(
    _config.apSsid.c_str(),
    _config.apPassword.c_str(),
    AP_CHANNEL,
    false,                    // Hidden SSID: false
    AP_MAX_CONNECTIONS
  );
  
  // Aguarda o AP iniciar
  delay(100);
  
  // Configura IP do AP (opcional, usa padrão 192.168.4.1)
  // WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  
  _apActive = true;
  
  DEBUG_PRINTF("[WiFi] AP iniciado! SSID: %s\n", _config.apSsid.c_str());
  DEBUG_PRINTF("[WiFi] AP IP: %s\n", WiFi.softAPIP().toString().c_str());
  
  return true;
}

bool WiFiManager::restartAP(const String& ssid, const String& password) {
  DEBUG_PRINTLN("[WiFi] Reiniciando Access Point...");
  
  // Atualiza configuração se fornecida
  if (ssid.length() > 0) {
    _config.apSsid = ssid;
  }
  if (password.length() >= 8) {
    _config.apPassword = password;
  }
  
  // Para o AP atual
  WiFi.softAPdisconnect(true);
  delay(100);
  
  // Reinicia
  return startAP();
}

// ═══════════════════════════════════════════════════════════════
// STATION (CLIENTE WIFI)
// ═══════════════════════════════════════════════════════════════

bool WiFiManager::startStation() {
  if (_config.staSsid.length() == 0) {
    DEBUG_PRINTLN("[WiFi] SSID não configurado para Station!");
    return false;
  }
  
  DEBUG_PRINTF("[WiFi] Conectando em: %s\n", _config.staSsid.c_str());
  
  WiFi.begin(_config.staSsid.c_str(), _config.staPassword.c_str());
  
  // Aguarda conexão com timeout
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < CONNECTION_TIMEOUT_MS) {
    delay(500);
    DEBUG_PRINT(".");
  }
  DEBUG_PRINTLN();
  
  if (WiFi.status() == WL_CONNECTED) {
    _staConnected = true;
    DEBUG_PRINTF("[WiFi] Conectado! IP: %s\n", WiFi.localIP().toString().c_str());
    return true;
  }
  
  DEBUG_PRINTLN("[WiFi] Falha na conexão!");
  _staConnected = false;
  return false;
}

bool WiFiManager::connectToWiFi(const String& ssid, const String& password) {
  _config.staSsid = ssid;
  _config.staPassword = password;
  _config.stationEnabled = true;
  
  // Salva a configuração
  saveConfig();
  
  // Tenta conectar
  return startStation();
}

void WiFiManager::disconnectStation() {
  DEBUG_PRINTLN("[WiFi] Desconectando da rede WiFi...");
  WiFi.disconnect(true);
  _staConnected = false;
  _config.stationEnabled = false;
  saveConfig();
}

// ═══════════════════════════════════════════════════════════════
// MDNS
// ═══════════════════════════════════════════════════════════════

bool WiFiManager::setupMDNS() {
  DEBUG_PRINTLN("[WiFi] Configurando mDNS...");
  
  if (!MDNS.begin(MDNS_HOSTNAME)) {
    DEBUG_PRINTLN("[WiFi] ERRO: Falha ao iniciar mDNS!");
    return false;
  }
  
  // Adiciona serviço HTTP
  MDNS.addService("http", "tcp", WEB_SERVER_PORT);
  
  DEBUG_PRINTF("[WiFi] mDNS iniciado: http://%s.local:%d\n", MDNS_HOSTNAME, WEB_SERVER_PORT);
  return true;
}

void WiFiManager::updateMDNS() {
  // No ESP32, o mDNS é atualizado automaticamente
  // Este método existe para compatibilidade futura
}

// ═══════════════════════════════════════════════════════════════
// CONFIGURAÇÃO (PERSISTÊNCIA)
// ═══════════════════════════════════════════════════════════════

bool WiFiManager::loadConfig() {
  DEBUG_PRINTLN("[WiFi] Carregando configuração...");
  
  if (!Storage.fileExists(WIFI_CONFIG_FILENAME)) {
    DEBUG_PRINTLN("[WiFi] Arquivo de configuração não existe. Usando padrões.");
    
    // Usa credenciais padrão do config.h se AUTO_CONNECT_WIFI estiver habilitado
    #ifdef AUTO_CONNECT_WIFI
    #if AUTO_CONNECT_WIFI == true
      _config.staSsid = DEFAULT_STA_SSID;
      _config.staPassword = DEFAULT_STA_PASSWORD;
      _config.stationEnabled = true;
      DEBUG_PRINTF("[WiFi] Usando credenciais padrão: %s\n", DEFAULT_STA_SSID);
    #endif
    #endif
    
    return false;
  }
  
  String content = Storage.readFile(WIFI_CONFIG_FILENAME);
  if (content.length() == 0) {
    DEBUG_PRINTLN("[WiFi] Arquivo de configuração vazio!");
    return false;
  }
  
  // Parse JSON
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, content);
  
  if (error) {
    DEBUG_PRINTF("[WiFi] ERRO ao parsear JSON: %s\n", error.c_str());
    return false;
  }
  
  // Carrega valores
  _config.staSsid = doc["sta_ssid"] | "";
  _config.staPassword = doc["sta_password"] | "";
  _config.apSsid = doc["ap_ssid"] | DEFAULT_AP_SSID;
  _config.apPassword = doc["ap_password"] | DEFAULT_AP_PASSWORD;
  _config.stationEnabled = doc["station_enabled"] | false;
  
  DEBUG_PRINTLN("[WiFi] Configuração carregada com sucesso!");
  return true;
}

bool WiFiManager::saveConfig() {
  DEBUG_PRINTLN("[WiFi] Salvando configuração...");
  
  StaticJsonDocument<512> doc;
  
  doc["sta_ssid"] = _config.staSsid;
  doc["sta_password"] = _config.staPassword;
  doc["ap_ssid"] = _config.apSsid;
  doc["ap_password"] = _config.apPassword;
  doc["station_enabled"] = _config.stationEnabled;
  
  String output;
  serializeJson(doc, output);
  
  if (Storage.writeFile(WIFI_CONFIG_FILENAME, output)) {
    DEBUG_PRINTLN("[WiFi] Configuração salva com sucesso!");
    return true;
  }
  
  DEBUG_PRINTLN("[WiFi] ERRO ao salvar configuração!");
  return false;
}

// ═══════════════════════════════════════════════════════════════
// STATUS E INFORMAÇÕES
// ═══════════════════════════════════════════════════════════════

WiFiConfig& WiFiManager::getConfig() {
  return _config;
}

WiFiStatus WiFiManager::getStatus() {
  WiFiStatus status;
  
  status.apActive = _apActive;
  status.staConnected = (WiFi.status() == WL_CONNECTED);
  status.apIP = WiFi.softAPIP().toString();
  status.staIP = WiFi.localIP().toString();
  status.staSsid = _config.staSsid;
  status.staRssi = WiFi.RSSI();
  status.hostname = MDNS_HOSTNAME;
  status.connectedClients = WiFi.softAPgetStationNum();
  
  return status;
}

String WiFiManager::getMainIP() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }
  return WiFi.softAPIP().toString();
}

bool WiFiManager::isStationConnected() {
  return (WiFi.status() == WL_CONNECTED);
}

bool WiFiManager::isAPActive() {
  return _apActive;
}

int WiFiManager::getSignalStrength() {
  if (!isStationConnected()) return 0;
  
  int rssi = WiFi.RSSI();
  // Converte RSSI para porcentagem (aproximado)
  // -50 dBm = 100%, -100 dBm = 0%
  int strength = 2 * (rssi + 100);
  return constrain(strength, 0, 100);
}

// ═══════════════════════════════════════════════════════════════
// SCAN DE REDES
// ═══════════════════════════════════════════════════════════════

String WiFiManager::scanNetworks() {
  DEBUG_PRINTLN("[WiFi] Escaneando redes...");
  
  int n = WiFi.scanNetworks();
  
  StaticJsonDocument<2048> doc;
  JsonArray networks = doc.createNestedArray("networks");
  
  for (int i = 0; i < n && i < 20; i++) {
    JsonObject net = networks.createNestedObject();
    net["ssid"] = WiFi.SSID(i);
    net["rssi"] = WiFi.RSSI(i);
    net["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "open" : "secured";
  }
  
  doc["count"] = n;
  
  WiFi.scanDelete(); // Limpa resultados
  
  String output;
  serializeJson(doc, output);
  
  DEBUG_PRINTF("[WiFi] %d redes encontradas.\n", n);
  return output;
}

// ═══════════════════════════════════════════════════════════════
// CALLBACK DE EVENTOS
// ═══════════════════════════════════════════════════════════════

void WiFiManager::onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      DEBUG_PRINTLN("[WiFi] Evento: IP obtido!");
      break;
      
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      DEBUG_PRINTLN("[WiFi] Evento: Desconectado da rede!");
      break;
      
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      DEBUG_PRINTLN("[WiFi] Evento: Cliente conectado ao AP!");
      break;
      
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      DEBUG_PRINTLN("[WiFi] Evento: Cliente desconectou do AP!");
      break;
      
    default:
      break;
  }
}

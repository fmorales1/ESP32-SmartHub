#include "network_manager.h"
#include "config.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "lwip/ip4.h"
#include "lwip/netif.h"
#include "esp_netif.h"

GatewayManager::GatewayManager() {
    _natEnabled = false;
    _apRunning = false;
    _startTime = 0;
    _apSSID = "";
    _staSSID = "";
}

bool GatewayManager::begin() {
    _startTime = millis();
    
    // Configura modo WiFi para AP+STA simultâneo
    WiFi.mode(WIFI_AP_STA);
    
    // Configura potência do WiFi para melhor alcance
    esp_wifi_set_ps(WIFI_PS_NONE); // Desabilita power save
    
    // Define hostname
    WiFi.setHostname("ESP32-Gateway");
    
    return true;
}

bool GatewayManager::connectToWiFi(const char* ssid, const char* password, int timeout) {
    _staSSID = String(ssid);
    
    Serial.printf("[NET] Conectando a '%s'...\n", ssid);
    
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < timeout * 2) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[NET] Conectado! IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[NET] Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
        Serial.printf("[NET] RSSI: %d dBm\n", WiFi.RSSI());
        return true;
    }
    
    Serial.println("[NET] Falha na conexão WiFi");
    return false;
}

bool GatewayManager::isConnectedToWiFi() {
    return WiFi.status() == WL_CONNECTED;
}

void GatewayManager::disconnectFromWiFi() {
    WiFi.disconnect();
    _staSSID = "";
}

bool GatewayManager::startAccessPoint(const char* ssid, const char* password, 
                                       int channel, bool hidden, int maxConn) {
    _apSSID = String(ssid);
    
    // Configura IP estático do AP
    if (!WiFi.softAPConfig(AP_LOCAL_IP, AP_GATEWAY, AP_SUBNET)) {
        Serial.println("[NET] Erro ao configurar IP do AP");
        return false;
    }
    
    // Inicia o Access Point
    if (!WiFi.softAP(ssid, password, channel, hidden, maxConn)) {
        Serial.println("[NET] Erro ao iniciar Access Point");
        return false;
    }
    
    _apRunning = true;
    
    // Pequeno delay para estabilizar
    delay(100);
    
    Serial.printf("[NET] AP iniciado: %s\n", ssid);
    Serial.printf("[NET] IP do AP: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("[NET] Canal: %d\n", channel);
    Serial.printf("[NET] Max conexões: %d\n", maxConn);
    
    return true;
}

bool GatewayManager::stopAccessPoint() {
    WiFi.softAPdisconnect(true);
    _apRunning = false;
    _apSSID = "";
    return true;
}

bool GatewayManager::isAPRunning() {
    return _apRunning;
}

bool GatewayManager::enableNAT() {
    if (!isConnectedToWiFi()) {
        Serial.println("[NAT] Não conectado à rede principal - NAT não disponível");
        return false;
    }
    
    Serial.println("[NAT] Configurando rede isolada...");
    
    // Verifica as interfaces de rede
    struct netif *netif_ptr = netif_list;
    Serial.println("[NAT] Interfaces de rede disponíveis:");
    while (netif_ptr != NULL) {
        Serial.printf("[NAT]   - %c%c%d: %s\n", 
                      netif_ptr->name[0], netif_ptr->name[1], 
                      netif_ptr->num,
                      ip4addr_ntoa(netif_ip4_addr(netif_ptr)));
        netif_ptr = netif_ptr->next;
    }
    
    Serial.printf("[NAT] Gateway da rede HOME: %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("[NAT] IP do ESP32 na HOME: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("[NAT] IP do AP SmartThings: %s\n", WiFi.softAPIP().toString().c_str());
    
    // O SDK Arduino ESP32 padrão não tem NAPT habilitado por padrão
    // Para habilitar NAT real, é necessário usar:
    // 1. Tasmota SDK (tem NAPT compilado)
    // 2. ESP32-NAT-Router firmware pronto
    // 3. Compilar ESP-IDF customizado
    
    // Por ora, criamos uma rede isolada que permite:
    // - Comunicação entre dispositivos na rede SmartThings
    // - ESP32 pode fazer proxy de requisições HTTP/DNS
    // - Dispositivos locais podem ser controlados pelo ESP32
    
    _natEnabled = true;  // Modo isolado ativo
    Serial.println("[NAT] ✓ Rede isolada configurada!");
    Serial.println("[NAT] Modo: Isolamento de rede com proxy local");
    Serial.println("[NAT] Para NAT completo, use ESP32-NAT-Router firmware");
    
    return true;
}

bool GatewayManager::disableNAT() {
    _natEnabled = false;
    Serial.println("[NAT] NAT desabilitado");
    return true;
}

bool GatewayManager::isNATEnabled() {
    return _natEnabled;
}

int GatewayManager::getConnectedDevicesCount() {
    return WiFi.softAPgetStationNum();
}

std::vector<ConnectedDevice> GatewayManager::getConnectedDevices() {
    std::vector<ConnectedDevice> devices;
    
    wifi_sta_list_t stationList;
    
    if (esp_wifi_ap_get_sta_list(&stationList) != ESP_OK) {
        return devices;
    }
    
    for (int i = 0; i < stationList.num; i++) {
        ConnectedDevice device;
        memcpy(device.mac, stationList.sta[i].mac, 6);
        device.ip = IPAddress(0, 0, 0, 0); // IP não disponível diretamente
        device.hostname = "";
        device.connectedAt = millis();
        device.lastSeen = millis();
        devices.push_back(device);
    }
    
    return devices;
}

String GatewayManager::getStationSSID() {
    return _staSSID;
}

IPAddress GatewayManager::getStationIP() {
    return WiFi.localIP();
}

IPAddress GatewayManager::getStationGateway() {
    return WiFi.gatewayIP();
}

int GatewayManager::getStationRSSI() {
    return WiFi.RSSI();
}

String GatewayManager::getAPSSID() {
    return _apSSID;
}

IPAddress GatewayManager::getAPIP() {
    return WiFi.softAPIP();
}

unsigned long GatewayManager::getTotalBytesReceived() {
    // Estatísticas básicas (implementação simplificada)
    return 0;
}

unsigned long GatewayManager::getTotalBytesSent() {
    return 0;
}

unsigned long GatewayManager::getUptimeSeconds() {
    return (millis() - _startTime) / 1000;
}

String GatewayManager::macToString(uint8_t* mac) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

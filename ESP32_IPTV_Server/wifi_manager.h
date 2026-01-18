/*
 * ═══════════════════════════════════════════════════════════════
 * ESP32 IPTV Server - Gerenciador WiFi (Header)
 * ═══════════════════════════════════════════════════════════════
 * 
 * Gerencia conexões WiFi em modo AP+STA simultâneo,
 * mDNS para descoberta de rede e persistência de credenciais.
 * 
 * ═══════════════════════════════════════════════════════════════
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include "config.h"

// ═══════════════════════════════════════════════════════════════
// ESTRUTURA DE CONFIGURAÇÃO WIFI
// ═══════════════════════════════════════════════════════════════

struct WiFiConfig {
  String staSsid;         // SSID da rede WiFi para conectar
  String staPassword;     // Senha da rede WiFi
  String apSsid;          // SSID do Access Point
  String apPassword;      // Senha do Access Point
  bool stationEnabled;    // Conectar em rede existente?
  
  WiFiConfig() {
    staSsid = "";
    staPassword = "";
    apSsid = DEFAULT_AP_SSID;
    apPassword = DEFAULT_AP_PASSWORD;
    stationEnabled = false;
  }
};

// ═══════════════════════════════════════════════════════════════
// ESTRUTURA DE STATUS WIFI
// ═══════════════════════════════════════════════════════════════

struct WiFiStatus {
  bool apActive;          // Access Point está ativo?
  bool staConnected;      // Conectado em rede WiFi?
  String apIP;            // IP do Access Point
  String staIP;           // IP na rede WiFi
  String staSsid;         // SSID conectado
  int staRssi;            // Força do sinal (dBm)
  String hostname;        // Nome mDNS
  int connectedClients;   // Número de clientes no AP
};

// ═══════════════════════════════════════════════════════════════
// CLASSE GERENCIADORA WIFI
// ═══════════════════════════════════════════════════════════════

class WiFiManager {
public:
  // Construtor
  WiFiManager();
  
  // Inicializa WiFi (AP + opcionalmente STA)
  bool begin();
  
  // Configura e conecta em uma rede WiFi
  bool connectToWiFi(const String& ssid, const String& password);
  
  // Desconecta da rede WiFi Station
  void disconnectStation();
  
  // Reinicia o Access Point
  bool restartAP(const String& ssid = "", const String& password = "");
  
  // Carrega configuração do arquivo
  bool loadConfig();
  
  // Salva configuração em arquivo
  bool saveConfig();
  
  // Retorna configuração atual
  WiFiConfig& getConfig();
  
  // Retorna status atual
  WiFiStatus getStatus();
  
  // Retorna IP principal (STA se conectado, senão AP)
  String getMainIP();
  
  // Verifica se está conectado na rede WiFi
  bool isStationConnected();
  
  // Verifica se Access Point está ativo
  bool isAPActive();
  
  // Configura mDNS
  bool setupMDNS();
  
  // Atualiza mDNS (chamar periodicamente)
  void updateMDNS();
  
  // Lista redes WiFi disponíveis
  String scanNetworks();
  
  // Retorna força do sinal em porcentagem
  int getSignalStrength();
  
private:
  WiFiConfig _config;
  bool _apActive;
  bool _staConnected;
  unsigned long _lastReconnectAttempt;
  int _reconnectAttempts;
  
  // Inicia o Access Point
  bool startAP();
  
  // Conecta como Station
  bool startStation();
  
  // Callback de evento WiFi
  static void onWiFiEvent(WiFiEvent_t event);
};

// Instância global
extern WiFiManager WifiMgr;

#endif // WIFI_MANAGER_H

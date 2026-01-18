/*
 * ESP32 WiFi Gateway - Smart Home Network Isolation
 * 
 * Este projeto cria uma rede WiFi secundária isolada para dispositivos smart home.
 * O ESP32 funciona como ponte entre sua rede principal e a rede "SmartThings".
 * 
 * Funcionalidades:
 * - Modo AP+STA simultâneo
 * - NAT (Network Address Translation) entre as redes
 * - Interface web para configuração
 * - Monitoramento de dispositivos conectados
 * - Isolamento de rede para segurança
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "esp_wifi.h"
#include "lwip/lwip_napt.h"
#include "lwip/err.h"
#include "config.h"
#include "web_interface.h"
#include "network_manager.h"

// Objetos globais
WebServer webServer(WEB_SERVER_PORT);
DNSServer dnsServer;
GatewayManager GatewayManager;

// Variáveis de estado
bool wifiConnected = false;
bool apStarted = false;
unsigned long lastStatusCheck = 0;
const unsigned long STATUS_CHECK_INTERVAL = 10000; // 10 segundos

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(1000);
    
    Serial.println();
    Serial.println("╔══════════════════════════════════════════╗");
    Serial.println("║    ESP32 WiFi Gateway - Smart Home       ║");
    Serial.println("║         Network Isolation System         ║");
    Serial.println("╚══════════════════════════════════════════╝");
    Serial.println();

    // Inicializa o gerenciador de rede
    if (GatewayManager.begin()) {
        Serial.println("[OK] Gerenciador de rede inicializado");
    } else {
        Serial.println("[ERRO] Falha ao inicializar gerenciador de rede");
    }

    // Conecta à rede WiFi principal
    Serial.println("[INFO] Conectando à rede principal...");
    wifiConnected = GatewayManager.connectToWiFi(WIFI_STA_SSID, WIFI_STA_PASSWORD);
    
    if (wifiConnected) {
        Serial.print("[OK] Conectado! IP na rede principal: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("[AVISO] Não foi possível conectar à rede principal");
        Serial.println("[INFO] Continuando apenas com Access Point...");
    }

    // Inicia o Access Point para dispositivos smart
    Serial.println("[INFO] Iniciando Access Point 'SmartThings'...");
    apStarted = GatewayManager.startAccessPoint(
        WIFI_AP_SSID, 
        WIFI_AP_PASSWORD, 
        WIFI_AP_CHANNEL,
        WIFI_AP_HIDDEN,
        WIFI_AP_MAX_CONNECTIONS
    );

    if (apStarted) {
        Serial.print("[OK] Access Point iniciado! IP: ");
        Serial.println(WiFi.softAPIP());
        Serial.print("[INFO] SSID: ");
        Serial.println(WIFI_AP_SSID);
        Serial.print("[INFO] Senha: ");
        Serial.println(WIFI_AP_PASSWORD);
    } else {
        Serial.println("[ERRO] Falha ao iniciar Access Point");
    }

    // Habilita NAT se conectado à rede principal
    if (wifiConnected) {
        Serial.println("[INFO] Habilitando NAT (Network Address Translation)...");
        if (GatewayManager.enableNAT()) {
            Serial.println("[OK] NAT habilitado - dispositivos podem acessar internet");
        } else {
            Serial.println("[AVISO] NAT não disponível - apenas rede local");
        }
    }

    // Configura e inicia servidor web
    setupWebInterface(webServer, GatewayManager);
    webServer.begin();
    Serial.println("[OK] Servidor web iniciado na porta 80");
    
    // Inicia servidor DNS para captive portal
    dnsServer.start(53, "*", AP_LOCAL_IP);

    Serial.println();
    Serial.println("════════════════════════════════════════════");
    Serial.println("Sistema pronto!");
    Serial.println();
    Serial.print("Acesse o painel: http://");
    Serial.println(WiFi.softAPIP());
    Serial.println();
    Serial.println("Conecte seus dispositivos smart à rede:");
    Serial.print("  SSID: ");
    Serial.println(WIFI_AP_SSID);
    Serial.print("  Senha: ");
    Serial.println(WIFI_AP_PASSWORD);
    Serial.println("════════════════════════════════════════════");
}

void loop() {
    // Processa requisições DNS e HTTP
    dnsServer.processNextRequest();
    webServer.handleClient();

    // Verifica status periodicamente
    if (millis() - lastStatusCheck >= STATUS_CHECK_INTERVAL) {
        lastStatusCheck = millis();
        
        // Verifica conexão com rede principal
        if (wifiConnected && WiFi.status() != WL_CONNECTED) {
            Serial.println("[AVISO] Conexão com rede principal perdida. Reconectando...");
            wifiConnected = GatewayManager.connectToWiFi(WIFI_STA_SSID, WIFI_STA_PASSWORD);
            if (wifiConnected) {
                GatewayManager.enableNAT();
            }
        }

        // Log de dispositivos conectados
        int numClients = WiFi.softAPgetStationNum();
        if (numClients > 0) {
            Serial.print("[INFO] Dispositivos conectados ao SmartThings: ");
            Serial.println(numClients);
        }
    }

    delay(10);
}

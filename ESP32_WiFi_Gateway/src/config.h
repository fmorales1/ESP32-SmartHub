#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// CONFIGURAÇÕES DA REDE PRINCIPAL (SUA CASA)
// ============================================
#define WIFI_STA_SSID "HOME"          // Nome da sua rede WiFi principal
#define WIFI_STA_PASSWORD "16202813A"      // Senha da sua rede WiFi

// ============================================
// CONFIGURAÇÕES DO ACCESS POINT (SMART THINGS)
// ============================================
#define WIFI_AP_SSID "SmartThings"            // Nome da rede para dispositivos smart
#define WIFI_AP_PASSWORD "smart12345"         // Senha da rede smart (mínimo 8 caracteres)
#define WIFI_AP_CHANNEL 6                     // Canal WiFi (1-13)
#define WIFI_AP_MAX_CONNECTIONS 8             // Máximo de dispositivos conectados
#define WIFI_AP_HIDDEN false                  // Rede oculta?

// ============================================
// CONFIGURAÇÕES DE REDE DO AP
// ============================================
#define AP_LOCAL_IP IPAddress(192, 168, 4, 1)
#define AP_GATEWAY IPAddress(192, 168, 4, 1)
#define AP_SUBNET IPAddress(255, 255, 255, 0)

// ============================================
// CONFIGURAÇÕES DO SERVIDOR WEB
// ============================================
#define WEB_SERVER_PORT 80

// ============================================
// CONFIGURAÇÕES DE DEBUG
// ============================================
#define DEBUG_MODE true
#define SERIAL_BAUD 115200

#endif // CONFIG_H

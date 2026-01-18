/*
 * ═══════════════════════════════════════════════════════════════
 * ESP32 IPTV Server - Arquivo de Configuração
 * ═══════════════════════════════════════════════════════════════
 * 
 * Este arquivo contém todas as constantes e configurações
 * globais do servidor IPTV.
 * 
 * Autor: ESP32 IPTV Project
 * Versão: 1.0.0
 * ═══════════════════════════════════════════════════════════════
 */

#ifndef CONFIG_H
#define CONFIG_H

// ═══════════════════════════════════════════════════════════════
// CONFIGURAÇÕES DE REDE
// ═══════════════════════════════════════════════════════════════

// Porta do servidor web
#define WEB_SERVER_PORT 8080

// Nome do host para mDNS (acessar em http://esp32.local:8080)
#define MDNS_HOSTNAME "esp32"

// Timeout de conexão em milissegundos
#define CONNECTION_TIMEOUT_MS 30000

// Número máximo de conexões simultâneas
#define MAX_CONNECTIONS 3

// ═══════════════════════════════════════════════════════════════
// CONFIGURAÇÕES DO ACCESS POINT
// ═══════════════════════════════════════════════════════════════

// SSID padrão do Access Point
#define DEFAULT_AP_SSID "ESP32_IPTV"

// Senha padrão do Access Point (mínimo 8 caracteres)
#define DEFAULT_AP_PASSWORD "12345678"

// Canal WiFi do AP (1-13)
#define AP_CHANNEL 1

// Máximo de estações conectadas ao AP
#define AP_MAX_CONNECTIONS 4

// ═══════════════════════════════════════════════════════════════
// CONFIGURAÇÕES DE ARMAZENAMENTO
// ═══════════════════════════════════════════════════════════════

// Usar LittleFS (mais eficiente) ao invés de SPIFFS
#define USE_LITTLEFS true

// Nome do arquivo de playlist no sistema de arquivos
#define PLAYLIST_FILENAME "/playlist.m3u8"

// Nome do arquivo de configuração WiFi
#define WIFI_CONFIG_FILENAME "/wifi_config.json"

// Tamanho máximo do arquivo de playlist (2 MB)
#define MAX_PLAYLIST_SIZE (2 * 1024 * 1024)

// Tamanho do buffer para leitura de arquivos
#define FILE_BUFFER_SIZE 1024

// ═══════════════════════════════════════════════════════════════
// CONFIGURAÇÕES DO PARSER M3U8
// ═══════════════════════════════════════════════════════════════

// Número máximo de canais suportados
#define MAX_CHANNELS 500

// Tamanho máximo do nome do canal
#define MAX_CHANNEL_NAME_LENGTH 128

// Tamanho máximo da URL do canal
#define MAX_CHANNEL_URL_LENGTH 512

// Tamanho máximo da URL do logo
#define MAX_LOGO_URL_LENGTH 256

// ═══════════════════════════════════════════════════════════════
// CONFIGURAÇÕES DE DEBUG
// ═══════════════════════════════════════════════════════════════

// Habilitar logs de debug no Serial
#define DEBUG_ENABLED true

// Velocidade da porta serial
#define SERIAL_BAUD_RATE 115200

// Macro para logging condicional
#if DEBUG_ENABLED
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(...)
#endif

// ═══════════════════════════════════════════════════════════════
// CONFIGURAÇÕES DE PERFORMANCE
// ═══════════════════════════════════════════════════════════════

// Core para tarefas de rede (ESP32 tem 2 cores: 0 e 1)
#define NETWORK_CORE 0

// Core para tarefas de processamento
#define PROCESSING_CORE 1

// Tamanho da stack para tarefas (em words)
#define TASK_STACK_SIZE 8192

// Intervalo de atualização do status (ms)
#define STATUS_UPDATE_INTERVAL 5000

// ═══════════════════════════════════════════════════════════════
// VERSÃO DO FIRMWARE
// ═══════════════════════════════════════════════════════════════

#define FIRMWARE_VERSION "1.0.0"
#define FIRMWARE_NAME "ESP32 IPTV Server"

#endif // CONFIG_H

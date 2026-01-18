/*
 * ═══════════════════════════════════════════════════════════════════════════════
 *                         ESP32 IPTV SERVER
 * ═══════════════════════════════════════════════════════════════════════════════
 * 
 * Servidor IPTV completo para ESP32 DevKit v4 / D1 Mini
 * 
 * FUNCIONALIDADES:
 * ✅ Servidor Web HTTP assíncrono (porta 8080)
 * ✅ Gerenciamento de playlist M3U8 via SPIFFS/LittleFS
 * ✅ Interface web responsiva para upload e gerenciamento
 * ✅ API REST para integração com players IPTV
 * ✅ mDNS para acesso em http://esp32.local:8080
 * ✅ WiFi AP+STA simultâneos
 * ✅ Persistência de configurações
 * 
 * HARDWARE NECESSÁRIO:
 * - ESP32 DevKit v4, D1 Mini, ou compatível
 * - 4MB Flash (mínimo)
 * - WiFi 2.4GHz
 * 
 * DEPENDÊNCIAS (Arduino IDE):
 * - ESP32 by Espressif (2.0.x+)
 * - ESPAsyncWebServer by me-no-dev
 * - AsyncTCP by me-no-dev  
 * - ArduinoJson by Benoit Blanchon (6.x)
 * 
 * AUTOR: ESP32 IPTV Project
 * VERSÃO: 1.0.0
 * DATA: 2024
 * 
 * ═══════════════════════════════════════════════════════════════════════════════
 */

// ═══════════════════════════════════════════════════════════════
// INCLUSÃO DE BIBLIOTECAS
// ═══════════════════════════════════════════════════════════════

// Configurações globais (DEVE VIR PRIMEIRO)
#include "config.h"

// Bibliotecas do sistema
#include <Arduino.h>

// Módulos do projeto
#include "storage_manager.h"
#include "playlist_parser.h"
#include "wifi_manager.h"
#include "web_server.h"

// ═══════════════════════════════════════════════════════════════
// VARIÁVEIS GLOBAIS
// ═══════════════════════════════════════════════════════════════

// Tempo da última atualização de status
unsigned long lastStatusUpdate = 0;

// Flag para indicar que o sistema está pronto
bool systemReady = false;

// ═══════════════════════════════════════════════════════════════
// PROTÓTIPOS DE FUNÇÕES
// ═══════════════════════════════════════════════════════════════

void printSystemInfo();
void printBanner();
void watchdogTask(void* parameter);

// ═══════════════════════════════════════════════════════════════
// SETUP - INICIALIZAÇÃO DO SISTEMA
// ═══════════════════════════════════════════════════════════════

void setup() {
  // ─────────────────────────────────────────────────────────────
  // 1. Inicializa comunicação serial para debug
  // ─────────────────────────────────────────────────────────────
  Serial.begin(SERIAL_BAUD_RATE);
  
  // Aguarda a porta serial estar pronta
  delay(1000);
  
  // Exibe banner inicial
  printBanner();
  
  // ─────────────────────────────────────────────────────────────
  // 2. Inicializa sistema de arquivos (LittleFS)
  // ─────────────────────────────────────────────────────────────
  DEBUG_PRINTLN("\n[SETUP] Inicializando sistema de arquivos...");
  
  if (!Storage.begin()) {
    DEBUG_PRINTLN("[SETUP] ERRO CRÍTICO: Falha no sistema de arquivos!");
    DEBUG_PRINTLN("[SETUP] Tentando formatar...");
    
    // Tenta formatar e reiniciar
    Storage.format();
    if (!Storage.begin()) {
      DEBUG_PRINTLN("[SETUP] Não foi possível recuperar. Reiniciando...");
      delay(3000);
      ESP.restart();
    }
  }
  
  // Lista arquivos existentes
  Storage.listFiles();
  
  // ─────────────────────────────────────────────────────────────
  // 3. Carrega playlist existente (se houver)
  // ─────────────────────────────────────────────────────────────
  DEBUG_PRINTLN("\n[SETUP] Verificando playlist salva...");
  
  if (Storage.fileExists(PLAYLIST_FILENAME)) {
    DEBUG_PRINTLN("[SETUP] Playlist encontrada! Carregando...");
    
    if (Playlist.parseFile(PLAYLIST_FILENAME)) {
      DEBUG_PRINTF("[SETUP] Playlist carregada: %d canais\n", Playlist.getChannelCount());
    } else {
      DEBUG_PRINTF("[SETUP] Erro ao carregar playlist: %s\n", Playlist.getErrorMessage().c_str());
    }
  } else {
    DEBUG_PRINTLN("[SETUP] Nenhuma playlist salva encontrada.");
  }
  
  // ─────────────────────────────────────────────────────────────
  // 4. Inicializa WiFi (AP + Station)
  // ─────────────────────────────────────────────────────────────
  DEBUG_PRINTLN("\n[SETUP] Inicializando WiFi...");
  
  if (!WifiMgr.begin()) {
    DEBUG_PRINTLN("[SETUP] AVISO: Problemas na inicialização WiFi!");
    // Continua mesmo assim, pode funcionar parcialmente
  }
  
  // Exibe informações de conexão
  WiFiStatus wifiStatus = WifiMgr.getStatus();
  DEBUG_PRINTLN("\n[SETUP] ═══ Informações de Rede ═══");
  DEBUG_PRINTF("  AP SSID: %s\n", DEFAULT_AP_SSID);
  DEBUG_PRINTF("  AP Senha: %s\n", DEFAULT_AP_PASSWORD);
  DEBUG_PRINTF("  AP IP: %s\n", wifiStatus.apIP.c_str());
  
  if (wifiStatus.staConnected) {
    DEBUG_PRINTF("  Station IP: %s\n", wifiStatus.staIP.c_str());
    DEBUG_PRINTF("  Conectado em: %s\n", wifiStatus.staSsid.c_str());
  }
  
  DEBUG_PRINTF("  mDNS: http://%s.local:%d\n", MDNS_HOSTNAME, WEB_SERVER_PORT);
  DEBUG_PRINTLN("═══════════════════════════════════\n");
  
  // ─────────────────────────────────────────────────────────────
  // 5. Inicializa servidor web
  // ─────────────────────────────────────────────────────────────
  DEBUG_PRINTLN("[SETUP] Iniciando servidor web...");
  WebServer.begin();
  
  // ─────────────────────────────────────────────────────────────
  // 6. Cria tarefa de monitoramento em segundo plano
  // ─────────────────────────────────────────────────────────────
  DEBUG_PRINTLN("[SETUP] Criando tarefa de monitoramento...");
  
  xTaskCreatePinnedToCore(
    watchdogTask,           // Função da tarefa
    "Watchdog",             // Nome da tarefa
    4096,                   // Tamanho da stack
    NULL,                   // Parâmetro
    1,                      // Prioridade (baixa)
    NULL,                   // Handle da tarefa
    PROCESSING_CORE         // Core 1 (processamento)
  );
  
  // ─────────────────────────────────────────────────────────────
  // 7. Sistema pronto!
  // ─────────────────────────────────────────────────────────────
  systemReady = true;
  
  DEBUG_PRINTLN("\n═══════════════════════════════════════════════════════════");
  DEBUG_PRINTLN("           🎉 ESP32 IPTV SERVER PRONTO! 🎉");
  DEBUG_PRINTLN("═══════════════════════════════════════════════════════════");
  DEBUG_PRINTLN("");
  DEBUG_PRINTF("  📡 Conecte-se ao WiFi: %s (senha: %s)\n", DEFAULT_AP_SSID, DEFAULT_AP_PASSWORD);
  DEBUG_PRINTF("  🌐 Acesse: http://%s:8080\n", WifiMgr.getMainIP().c_str());
  DEBUG_PRINTF("  🔗 Ou: http://%s.local:%d\n", MDNS_HOSTNAME, WEB_SERVER_PORT);
  DEBUG_PRINTLN("");
  DEBUG_PRINTLN("═══════════════════════════════════════════════════════════\n");
  
  // Exibe informações do sistema
  printSystemInfo();
}

// ═══════════════════════════════════════════════════════════════
// LOOP PRINCIPAL
// ═══════════════════════════════════════════════════════════════

void loop() {
  // O AsyncWebServer não precisa de chamadas no loop
  // Apenas atualizamos o mDNS periodicamente
  
  // Atualiza mDNS
  WifiMgr.updateMDNS();
  
  // Log de status periódico (a cada 30 segundos)
  if (millis() - lastStatusUpdate >= 30000) {
    lastStatusUpdate = millis();
    
    DEBUG_PRINTF("[STATUS] Uptime: %lu s | RAM livre: %lu bytes | Canais: %d\n",
                 millis() / 1000,
                 ESP.getFreeHeap(),
                 Playlist.getChannelCount());
  }
  
  // Pequeno delay para economizar energia e evitar WDT
  delay(10);
}

// ═══════════════════════════════════════════════════════════════
// FUNÇÕES AUXILIARES
// ═══════════════════════════════════════════════════════════════

/**
 * Exibe banner inicial no Serial
 */
void printBanner() {
  DEBUG_PRINTLN("\n\n");
  DEBUG_PRINTLN("═══════════════════════════════════════════════════════════");
  DEBUG_PRINTLN("          ███████╗███████╗██████╗ ██████╗ ██████╗          ");
  DEBUG_PRINTLN("          ██╔════╝██╔════╝██╔══██╗╚════██╗╚════██╗         ");
  DEBUG_PRINTLN("          █████╗  ███████╗██████╔╝ █████╔╝ █████╔╝         ");
  DEBUG_PRINTLN("          ██╔══╝  ╚════██║██╔═══╝  ╚═══██╗██╔═══╝          ");
  DEBUG_PRINTLN("          ███████╗███████║██║     ██████╔╝███████╗         ");
  DEBUG_PRINTLN("          ╚══════╝╚══════╝╚═╝     ╚═════╝ ╚══════╝         ");
  DEBUG_PRINTLN("                                                           ");
  DEBUG_PRINTLN("                    📺 IPTV SERVER 📺                      ");
  DEBUG_PRINTLN("═══════════════════════════════════════════════════════════");
  DEBUG_PRINTF("  Versão: %s\n", FIRMWARE_VERSION);
  DEBUG_PRINTF("  Compilado em: %s %s\n", __DATE__, __TIME__);
  DEBUG_PRINTLN("═══════════════════════════════════════════════════════════\n");
}

/**
 * Exibe informações detalhadas do sistema
 */
void printSystemInfo() {
  DEBUG_PRINTLN("\n[INFO] ═══ Informações do Sistema ═══");
  
  // Chip info
  DEBUG_PRINTF("  Chip: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  DEBUG_PRINTF("  Cores: %d\n", ESP.getChipCores());
  DEBUG_PRINTF("  Frequência: %d MHz\n", ESP.getCpuFreqMHz());
  
  // Memória
  DEBUG_PRINTF("  RAM Total: %lu bytes\n", ESP.getHeapSize());
  DEBUG_PRINTF("  RAM Livre: %lu bytes\n", ESP.getFreeHeap());
  DEBUG_PRINTF("  RAM Mínima: %lu bytes\n", ESP.getMinFreeHeap());
  
  // Flash
  DEBUG_PRINTF("  Flash: %lu bytes\n", ESP.getFlashChipSize());
  DEBUG_PRINTF("  Flash Speed: %d MHz\n", ESP.getFlashChipSpeed() / 1000000);
  
  // Sistema de arquivos
  DEBUG_PRINTF("  FS Total: %lu bytes\n", Storage.getTotalSpace());
  DEBUG_PRINTF("  FS Usado: %lu bytes\n", Storage.getUsedSpace());
  DEBUG_PRINTF("  FS Livre: %lu bytes\n", Storage.getFreeSpace());
  
  DEBUG_PRINTLN("═══════════════════════════════════════\n");
}

/**
 * Tarefa de monitoramento em segundo plano
 * Roda no Core 1 e monitora a saúde do sistema
 */
void watchdogTask(void* parameter) {
  const TickType_t xDelay = pdMS_TO_TICKS(10000); // 10 segundos
  
  while (true) {
    // Verifica memória disponível
    size_t freeHeap = ESP.getFreeHeap();
    
    if (freeHeap < 10000) {
      DEBUG_PRINTLN("[WATCHDOG] ⚠️ AVISO: Memória baixa!");
      DEBUG_PRINTF("[WATCHDOG] RAM livre: %lu bytes\n", freeHeap);
    }
    
    // Verifica conexão WiFi Station e tenta reconectar
    if (WifiMgr.getConfig().stationEnabled && !WifiMgr.isStationConnected()) {
      DEBUG_PRINTLN("[WATCHDOG] WiFi Station desconectado. Tentando reconectar...");
      // Não reconecta automaticamente para evitar problemas
      // O usuário pode reconectar pela interface web
    }
    
    // Aguarda próximo ciclo
    vTaskDelay(xDelay);
  }
}

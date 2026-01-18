/*
 * ═══════════════════════════════════════════════════════════════
 * ESP32 IPTV Server - Servidor Web (Header)
 * ═══════════════════════════════════════════════════════════════
 * 
 * Implementa o servidor web assíncrono com todos os endpoints:
 * - Interface web HTML
 * - Upload de playlist M3U8
 * - API REST
 * - Proxy de streams
 * 
 * ═══════════════════════════════════════════════════════════════
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "config.h"

// ═══════════════════════════════════════════════════════════════
// CLASSE DO SERVIDOR WEB
// ═══════════════════════════════════════════════════════════════

class WebServerManager {
public:
  // Construtor
  WebServerManager();
  
  // Inicializa o servidor
  void begin();
  
  // Para o servidor
  void stop();
  
  // Verifica se está rodando
  bool isRunning();

private:
  AsyncWebServer* _server;
  bool _running;
  
  // Configura todas as rotas
  void setupRoutes();
  
  // === HANDLERS DE ROTA ===
  
  // GET / - Interface web principal
  static void handleRoot(AsyncWebServerRequest* request);
  
  // GET /style.css - Estilos CSS
  static void handleCSS(AsyncWebServerRequest* request);
  
  // GET /script.js - JavaScript
  static void handleJS(AsyncWebServerRequest* request);
  
  // POST /upload - Upload de arquivo M3U8
  static void handleUpload(AsyncWebServerRequest* request, 
                           String filename, 
                           size_t index, 
                           uint8_t* data, 
                           size_t len, 
                           bool final);
  
  // GET /playlist.m3u8 - Retorna arquivo bruto
  static void handlePlaylist(AsyncWebServerRequest* request);
  
  // GET /list.json - Lista de canais em JSON
  static void handleListJSON(AsyncWebServerRequest* request);
  
  // GET /api/status - Status do sistema
  static void handleStatus(AsyncWebServerRequest* request);
  
  // GET /api/wifi/scan - Escaneia redes WiFi
  static void handleWiFiScan(AsyncWebServerRequest* request);
  
  // POST /api/wifi/connect - Conecta em rede WiFi
  static void handleWiFiConnect(AsyncWebServerRequest* request, 
                                 uint8_t* data, 
                                 size_t len, 
                                 size_t index, 
                                 size_t total);
  
  // GET /proxy - Proxy de stream
  static void handleProxy(AsyncWebServerRequest* request);
  
  // DELETE /playlist - Apaga a playlist
  static void handleDeletePlaylist(AsyncWebServerRequest* request);
  
  // Handler para 404
  static void handleNotFound(AsyncWebServerRequest* request);
  
  // === CONTEÚDO HTML/CSS/JS ===
  
  static String getHTML();
  static String getCSS();
  static String getJS();
};

// Instância global
extern WebServerManager WebServer;

#endif // WEB_SERVER_H

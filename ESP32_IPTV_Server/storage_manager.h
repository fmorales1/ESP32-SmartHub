/*
 * ═══════════════════════════════════════════════════════════════
 * ESP32 IPTV Server - Gerenciador de Armazenamento (Header)
 * ═══════════════════════════════════════════════════════════════
 * 
 * Responsável por todas as operações de sistema de arquivos
 * usando LittleFS (ou SPIFFS como fallback).
 * 
 * ═══════════════════════════════════════════════════════════════
 */

#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <Arduino.h>
#include <FS.h>

#if USE_LITTLEFS
  #include <LittleFS.h>
  #define FILESYSTEM LittleFS
#else
  #include <SPIFFS.h>
  #define FILESYSTEM SPIFFS
#endif

#include "config.h"

// ═══════════════════════════════════════════════════════════════
// ESTRUTURA DE INFORMAÇÕES DO ARQUIVO
// ═══════════════════════════════════════════════════════════════

struct FileInfo {
  String name;
  size_t size;
  bool exists;
};

// ═══════════════════════════════════════════════════════════════
// CLASSE GERENCIADORA DE ARMAZENAMENTO
// ═══════════════════════════════════════════════════════════════

class StorageManager {
public:
  // Inicializa o sistema de arquivos
  bool begin();
  
  // Finaliza o sistema de arquivos
  void end();
  
  // Verifica se um arquivo existe
  bool fileExists(const char* path);
  
  // Obtém tamanho de um arquivo
  size_t getFileSize(const char* path);
  
  // Obtém informações de um arquivo
  FileInfo getFileInfo(const char* path);
  
  // Lê conteúdo de um arquivo para String
  String readFile(const char* path);
  
  // Escreve String em um arquivo
  bool writeFile(const char* path, const String& content);
  
  // Apaga um arquivo
  bool deleteFile(const char* path);
  
  // Abre arquivo para leitura
  File openFileForRead(const char* path);
  
  // Abre arquivo para escrita
  File openFileForWrite(const char* path);
  
  // Obtém espaço total do sistema de arquivos
  size_t getTotalSpace();
  
  // Obtém espaço usado
  size_t getUsedSpace();
  
  // Obtém espaço livre
  size_t getFreeSpace();
  
  // Lista todos os arquivos
  void listFiles();
  
  // Formata o sistema de arquivos (CUIDADO!)
  bool format();
  
  // Verifica se o sistema de arquivos foi iniciado
  bool isReady();

private:
  bool _initialized = false;
};

// Instância global do gerenciador de armazenamento
extern StorageManager Storage;

#endif // STORAGE_MANAGER_H

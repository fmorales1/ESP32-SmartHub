/*
 * ═══════════════════════════════════════════════════════════════
 * ESP32 IPTV Server - Gerenciador de Armazenamento (Implementação)
 * ═══════════════════════════════════════════════════════════════
 */

#include "storage_manager.h"

// Instância global
StorageManager Storage;

// ═══════════════════════════════════════════════════════════════
// INICIALIZAÇÃO
// ═══════════════════════════════════════════════════════════════

bool StorageManager::begin() {
  DEBUG_PRINTLN("[Storage] Iniciando sistema de arquivos...");
  
  // Tenta montar o sistema de arquivos
  // O parâmetro true formata caso não exista
  if (!FILESYSTEM.begin(true)) {
    DEBUG_PRINTLN("[Storage] ERRO: Falha ao montar sistema de arquivos!");
    return false;
  }
  
  _initialized = true;
  
  // Log das informações do sistema de arquivos
  DEBUG_PRINTF("[Storage] Sistema de arquivos montado com sucesso!\n");
  DEBUG_PRINTF("[Storage] Espaço total: %lu bytes\n", getTotalSpace());
  DEBUG_PRINTF("[Storage] Espaço usado: %lu bytes\n", getUsedSpace());
  DEBUG_PRINTF("[Storage] Espaço livre: %lu bytes\n", getFreeSpace());
  
  return true;
}

void StorageManager::end() {
  if (_initialized) {
    FILESYSTEM.end();
    _initialized = false;
    DEBUG_PRINTLN("[Storage] Sistema de arquivos desmontado.");
  }
}

// ═══════════════════════════════════════════════════════════════
// OPERAÇÕES DE ARQUIVO
// ═══════════════════════════════════════════════════════════════

bool StorageManager::fileExists(const char* path) {
  if (!_initialized) return false;
  return FILESYSTEM.exists(path);
}

size_t StorageManager::getFileSize(const char* path) {
  if (!_initialized || !fileExists(path)) return 0;
  
  File file = FILESYSTEM.open(path, "r");
  if (!file) return 0;
  
  size_t size = file.size();
  file.close();
  return size;
}

FileInfo StorageManager::getFileInfo(const char* path) {
  FileInfo info;
  info.name = path;
  info.exists = fileExists(path);
  info.size = info.exists ? getFileSize(path) : 0;
  return info;
}

String StorageManager::readFile(const char* path) {
  if (!_initialized) {
    DEBUG_PRINTLN("[Storage] ERRO: Sistema não inicializado!");
    return "";
  }
  
  File file = FILESYSTEM.open(path, "r");
  if (!file) {
    DEBUG_PRINTF("[Storage] ERRO: Não foi possível abrir arquivo: %s\n", path);
    return "";
  }
  
  String content = file.readString();
  file.close();
  
  DEBUG_PRINTF("[Storage] Arquivo lido: %s (%d bytes)\n", path, content.length());
  return content;
}

bool StorageManager::writeFile(const char* path, const String& content) {
  if (!_initialized) {
    DEBUG_PRINTLN("[Storage] ERRO: Sistema não inicializado!");
    return false;
  }
  
  // Verifica se há espaço suficiente
  if (content.length() > getFreeSpace()) {
    DEBUG_PRINTLN("[Storage] ERRO: Espaço insuficiente!");
    return false;
  }
  
  File file = FILESYSTEM.open(path, "w");
  if (!file) {
    DEBUG_PRINTF("[Storage] ERRO: Não foi possível criar arquivo: %s\n", path);
    return false;
  }
  
  size_t written = file.print(content);
  file.close();
  
  if (written != content.length()) {
    DEBUG_PRINTLN("[Storage] ERRO: Escrita incompleta!");
    return false;
  }
  
  DEBUG_PRINTF("[Storage] Arquivo salvo: %s (%d bytes)\n", path, written);
  return true;
}

bool StorageManager::deleteFile(const char* path) {
  if (!_initialized) return false;
  
  if (!fileExists(path)) {
    DEBUG_PRINTF("[Storage] Arquivo não existe: %s\n", path);
    return true; // Considera sucesso se não existe
  }
  
  if (FILESYSTEM.remove(path)) {
    DEBUG_PRINTF("[Storage] Arquivo apagado: %s\n", path);
    return true;
  }
  
  DEBUG_PRINTF("[Storage] ERRO: Não foi possível apagar: %s\n", path);
  return false;
}

File StorageManager::openFileForRead(const char* path) {
  if (!_initialized) return File();
  return FILESYSTEM.open(path, "r");
}

File StorageManager::openFileForWrite(const char* path) {
  if (!_initialized) return File();
  return FILESYSTEM.open(path, "w");
}

// ═══════════════════════════════════════════════════════════════
// INFORMAÇÕES DO SISTEMA DE ARQUIVOS
// ═══════════════════════════════════════════════════════════════

size_t StorageManager::getTotalSpace() {
  if (!_initialized) return 0;
  return FILESYSTEM.totalBytes();
}

size_t StorageManager::getUsedSpace() {
  if (!_initialized) return 0;
  return FILESYSTEM.usedBytes();
}

size_t StorageManager::getFreeSpace() {
  return getTotalSpace() - getUsedSpace();
}

void StorageManager::listFiles() {
  if (!_initialized) {
    DEBUG_PRINTLN("[Storage] Sistema não inicializado!");
    return;
  }
  
  DEBUG_PRINTLN("\n[Storage] === LISTA DE ARQUIVOS ===");
  
  File root = FILESYSTEM.open("/");
  if (!root || !root.isDirectory()) {
    DEBUG_PRINTLN("[Storage] Não foi possível abrir diretório raiz!");
    return;
  }
  
  File file = root.openNextFile();
  int count = 0;
  
  while (file) {
    DEBUG_PRINTF("  - %s (%lu bytes)\n", file.name(), file.size());
    count++;
    file = root.openNextFile();
  }
  
  DEBUG_PRINTF("[Storage] Total: %d arquivo(s)\n", count);
  DEBUG_PRINTLN("================================\n");
}

bool StorageManager::format() {
  DEBUG_PRINTLN("[Storage] ATENÇÃO: Formatando sistema de arquivos...");
  
  if (FILESYSTEM.format()) {
    DEBUG_PRINTLN("[Storage] Formatação concluída!");
    return true;
  }
  
  DEBUG_PRINTLN("[Storage] ERRO: Falha na formatação!");
  return false;
}

bool StorageManager::isReady() {
  return _initialized;
}

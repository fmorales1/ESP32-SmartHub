/*
 * ═══════════════════════════════════════════════════════════════
 * ESP32 IPTV Server - Parser de Playlist M3U8 (Implementação)
 * ═══════════════════════════════════════════════════════════════
 */

#include "playlist_parser.h"
#include "storage_manager.h"
#include <ArduinoJson.h>

// Instância global
PlaylistParser Playlist;

// ═══════════════════════════════════════════════════════════════
// CONSTRUTOR
// ═══════════════════════════════════════════════════════════════

PlaylistParser::PlaylistParser() {
  _isValid = false;
  _errorMessage = "";
}

// ═══════════════════════════════════════════════════════════════
// PARSE DE STRING M3U8
// ═══════════════════════════════════════════════════════════════

bool PlaylistParser::parse(const String& content) {
  clear();
  
  DEBUG_PRINTLN("[Parser] Iniciando parse da playlist...");
  
  // Verifica se está vazio
  if (content.length() == 0) {
    _errorMessage = "Conteúdo vazio";
    DEBUG_PRINTLN("[Parser] ERRO: Conteúdo vazio!");
    return false;
  }
  
  // Verifica header M3U
  if (!content.startsWith("#EXTM3U")) {
    _errorMessage = "Header #EXTM3U não encontrado";
    DEBUG_PRINTLN("[Parser] ERRO: Header #EXTM3U não encontrado!");
    return false;
  }
  
  // Variáveis para parse linha a linha
  String currentLine = "";
  String extinfoLine = "";
  bool expectingUrl = false;
  int lineNumber = 0;
  
  // Percorre o conteúdo caractere por caractere
  for (size_t i = 0; i < content.length(); i++) {
    char c = content.charAt(i);
    
    // Fim de linha
    if (c == '\n' || c == '\r') {
      if (currentLine.length() > 0) {
        lineNumber++;
        currentLine = trim(currentLine);
        
        // Linha EXTINF (informações do canal)
        if (currentLine.startsWith("#EXTINF:")) {
          extinfoLine = currentLine;
          expectingUrl = true;
        }
        // Linha de URL (após EXTINF)
        else if (expectingUrl && !currentLine.startsWith("#")) {
          if (isValidUrl(currentLine)) {
            // Extrai informações do canal
            Channel ch;
            ch.name = extractChannelName(extinfoLine);
            ch.url = currentLine;
            ch.logo = extractAttribute(extinfoLine, "tvg-logo");
            ch.group = extractAttribute(extinfoLine, "group-title");
            
            // Se não tem nome, usa parte da URL
            if (ch.name.length() == 0) {
              ch.name = "Canal " + String(_channels.size() + 1);
            }
            
            // Adiciona à lista (respeitando limite)
            if (_channels.size() < MAX_CHANNELS) {
              _channels.push_back(ch);
              DEBUG_PRINTF("[Parser] Canal %d: %s\n", _channels.size(), ch.name.c_str());
            } else {
              DEBUG_PRINTLN("[Parser] AVISO: Limite de canais atingido!");
              break;
            }
          }
          expectingUrl = false;
          extinfoLine = "";
        }
        // Ignora outras linhas (comentários, etc)
        else if (!currentLine.startsWith("#")) {
          expectingUrl = false;
        }
        
        currentLine = "";
      }
    } else {
      currentLine += c;
    }
  }
  
  // Processa última linha se houver
  if (expectingUrl && currentLine.length() > 0) {
    currentLine = trim(currentLine);
    if (isValidUrl(currentLine)) {
      Channel ch;
      ch.name = extractChannelName(extinfoLine);
      ch.url = currentLine;
      ch.logo = extractAttribute(extinfoLine, "tvg-logo");
      ch.group = extractAttribute(extinfoLine, "group-title");
      
      if (ch.name.length() == 0) {
        ch.name = "Canal " + String(_channels.size() + 1);
      }
      
      if (_channels.size() < MAX_CHANNELS) {
        _channels.push_back(ch);
      }
    }
  }
  
  _isValid = (_channels.size() > 0);
  
  if (_isValid) {
    DEBUG_PRINTF("[Parser] Parse concluído! %d canais encontrados.\n", _channels.size());
  } else {
    _errorMessage = "Nenhum canal válido encontrado";
    DEBUG_PRINTLN("[Parser] AVISO: Nenhum canal válido encontrado.");
  }
  
  return _isValid;
}

// ═══════════════════════════════════════════════════════════════
// PARSE DE ARQUIVO
// ═══════════════════════════════════════════════════════════════

bool PlaylistParser::parseFile(const char* path) {
  DEBUG_PRINTF("[Parser] Lendo arquivo: %s\n", path);
  
  if (!Storage.fileExists(path)) {
    _errorMessage = "Arquivo não encontrado";
    DEBUG_PRINTLN("[Parser] ERRO: Arquivo não encontrado!");
    return false;
  }
  
  String content = Storage.readFile(path);
  return parse(content);
}

// ═══════════════════════════════════════════════════════════════
// OPERAÇÕES COM CANAIS
// ═══════════════════════════════════════════════════════════════

void PlaylistParser::clear() {
  _channels.clear();
  _isValid = false;
  _errorMessage = "";
}

size_t PlaylistParser::getChannelCount() {
  return _channels.size();
}

std::vector<Channel>& PlaylistParser::getChannels() {
  return _channels;
}

Channel* PlaylistParser::getChannel(size_t index) {
  if (index < _channels.size()) {
    return &_channels[index];
  }
  return nullptr;
}

std::vector<size_t> PlaylistParser::searchChannels(const String& query) {
  std::vector<size_t> results;
  String lowerQuery = query;
  lowerQuery.toLowerCase();
  
  for (size_t i = 0; i < _channels.size(); i++) {
    String lowerName = _channels[i].name;
    lowerName.toLowerCase();
    
    if (lowerName.indexOf(lowerQuery) >= 0) {
      results.push_back(i);
    }
  }
  
  return results;
}

// ═══════════════════════════════════════════════════════════════
// CONVERSÃO PARA JSON
// ═══════════════════════════════════════════════════════════════

String PlaylistParser::toJSON() {
  // Calcula tamanho necessário para o JSON
  // Cada canal precisa de aproximadamente 500 bytes
  size_t jsonSize = 256 + (_channels.size() * 600);
  
  // Limita o tamanho máximo do documento JSON
  DynamicJsonDocument doc(min(jsonSize, (size_t)65536));
  
  JsonArray channels = doc.createNestedArray("channels");
  
  for (size_t i = 0; i < _channels.size(); i++) {
    // Verifica se ainda há memória disponível
    if (doc.overflowed()) {
      DEBUG_PRINTLN("[Parser] AVISO: JSON truncado por falta de memória!");
      break;
    }
    
    JsonObject channel = channels.createNestedObject();
    channel["name"] = _channels[i].name;
    channel["url"] = _channels[i].url;
    channel["logo"] = _channels[i].logo;
    channel["group"] = _channels[i].group;
  }
  
  doc["total"] = _channels.size();
  
  String output;
  serializeJson(doc, output);
  return output;
}

// ═══════════════════════════════════════════════════════════════
// VALIDAÇÃO
// ═══════════════════════════════════════════════════════════════

bool PlaylistParser::isValid() {
  return _isValid;
}

String PlaylistParser::getErrorMessage() {
  return _errorMessage;
}

// ═══════════════════════════════════════════════════════════════
// MÉTODOS AUXILIARES PRIVADOS
// ═══════════════════════════════════════════════════════════════

String PlaylistParser::extractAttribute(const String& line, const String& attr) {
  // Procura por attr="valor" ou attr='valor'
  String searchDouble = attr + "=\"";
  String searchSingle = attr + "='";
  
  int startPos = line.indexOf(searchDouble);
  char endChar = '"';
  
  if (startPos < 0) {
    startPos = line.indexOf(searchSingle);
    endChar = '\'';
  }
  
  if (startPos < 0) return "";
  
  startPos += attr.length() + 2; // Pula attr="
  int endPos = line.indexOf(endChar, startPos);
  
  if (endPos < 0) return "";
  
  return line.substring(startPos, endPos);
}

String PlaylistParser::extractChannelName(const String& line) {
  // O nome do canal vem após a última vírgula
  int commaPos = line.lastIndexOf(',');
  
  if (commaPos < 0) return "";
  
  String name = line.substring(commaPos + 1);
  return trim(name);
}

bool PlaylistParser::isValidUrl(const String& url) {
  // Verifica se começa com http:// ou https://
  return (url.startsWith("http://") || url.startsWith("https://"));
}

String PlaylistParser::trim(const String& str) {
  String result = str;
  
  // Remove espaços no início
  while (result.length() > 0 && (result.charAt(0) == ' ' || result.charAt(0) == '\t')) {
    result = result.substring(1);
  }
  
  // Remove espaços no fim
  while (result.length() > 0 && 
         (result.charAt(result.length() - 1) == ' ' || 
          result.charAt(result.length() - 1) == '\t' ||
          result.charAt(result.length() - 1) == '\r')) {
    result = result.substring(0, result.length() - 1);
  }
  
  return result;
}

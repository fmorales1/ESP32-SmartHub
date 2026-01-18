/*
 * ═══════════════════════════════════════════════════════════════
 * ESP32 IPTV Server - Parser de Playlist M3U8 (Header)
 * ═══════════════════════════════════════════════════════════════
 * 
 * Responsável por fazer parse de arquivos M3U8 e extrair
 * informações dos canais (nome, URL, logo).
 * 
 * Formato M3U8 suportado:
 * #EXTM3U
 * #EXTINF:-1 tvg-logo="url" group-title="Grupo",Nome do Canal
 * http://url.do.stream
 * 
 * ═══════════════════════════════════════════════════════════════
 */

#ifndef PLAYLIST_PARSER_H
#define PLAYLIST_PARSER_H

#include <Arduino.h>
#include <vector>
#include "config.h"

// ═══════════════════════════════════════════════════════════════
// ESTRUTURA DE CANAL
// ═══════════════════════════════════════════════════════════════

struct Channel {
  String name;      // Nome do canal
  String url;       // URL do stream
  String logo;      // URL do logo (opcional)
  String group;     // Grupo/Categoria (opcional)
  
  // Construtor padrão
  Channel() : name(""), url(""), logo(""), group("") {}
  
  // Construtor com parâmetros
  Channel(const String& n, const String& u, const String& l = "", const String& g = "")
    : name(n), url(u), logo(l), group(g) {}
};

// ═══════════════════════════════════════════════════════════════
// CLASSE PARSER DE PLAYLIST
// ═══════════════════════════════════════════════════════════════

class PlaylistParser {
public:
  // Construtor
  PlaylistParser();
  
  // Faz parse de uma string M3U8 completa
  bool parse(const String& content);
  
  // Faz parse de um arquivo M3U8
  bool parseFile(const char* path);
  
  // Limpa a lista de canais
  void clear();
  
  // Retorna número de canais
  size_t getChannelCount();
  
  // Retorna lista de canais
  std::vector<Channel>& getChannels();
  
  // Retorna um canal específico por índice
  Channel* getChannel(size_t index);
  
  // Busca canais por nome (retorna índices)
  std::vector<size_t> searchChannels(const String& query);
  
  // Retorna JSON com lista de canais
  String toJSON();
  
  // Verifica se a playlist é válida
  bool isValid();
  
  // Retorna mensagem de erro (se houver)
  String getErrorMessage();

private:
  std::vector<Channel> _channels;
  bool _isValid;
  String _errorMessage;
  
  // Extrai valor de um atributo (ex: tvg-logo="valor")
  String extractAttribute(const String& line, const String& attr);
  
  // Extrai nome do canal da linha EXTINF
  String extractChannelName(const String& line);
  
  // Valida se é uma URL válida
  bool isValidUrl(const String& url);
  
  // Remove espaços em branco extras
  String trim(const String& str);
};

// Instância global do parser
extern PlaylistParser Playlist;

#endif // PLAYLIST_PARSER_H

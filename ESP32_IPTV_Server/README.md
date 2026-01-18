# 📺 ESP32 IPTV Server

Servidor IPTV completo para ESP32, permitindo gerenciar e servir playlists M3U8 em sua rede local.

![ESP32](https://img.shields.io/badge/ESP32-DevKit-blue)
![Version](https://img.shields.io/badge/version-1.0.0-green)
![License](https://img.shields.io/badge/license-MIT-orange)

---

## 📋 Índice

- [Funcionalidades](#-funcionalidades)
- [Hardware Necessário](#-hardware-necessário)
- [Instalação](#-instalação)
- [Configuração do Arduino IDE](#-configuração-do-arduino-ide)
- [Como Usar](#-como-usar)
- [API Endpoints](#-api-endpoints)
- [Arquitetura](#-arquitetura)
- [Troubleshooting](#-troubleshooting)
- [Melhorias Futuras](#-melhorias-futuras)

---

## ✨ Funcionalidades

### 🌐 Servidor Web
- Servidor HTTP assíncrono na porta 8080
- Interface web responsiva (mobile + desktop)
- Upload de arquivos M3U8 via browser
- Suporte a múltiplas conexões simultâneas

### 📁 Gerenciamento de Playlist
- Armazenamento em LittleFS (mais eficiente que SPIFFS)
- Suporte a playlists de até 2MB
- Parser inteligente de M3U8/M3U
- Extração de nome, URL, logo e grupo dos canais

### 📡 WiFi Dual-Mode
- Access Point + Station simultâneos
- AP padrão: `ESP32_IPTV` / senha: `12345678`
- Conexão em rede WiFi existente
- Scan de redes disponíveis
- mDNS: acesse em `http://esp32.local:8080`

### 🔧 API REST
- `/playlist.m3u8` - Playlist compatível com VLC/Kodi
- `/list.json` - Lista de canais em JSON
- `/api/status` - Status do sistema
- `/api/wifi/scan` - Redes disponíveis

---

## 🔧 Hardware Necessário

| Componente | Especificação |
|------------|---------------|
| ESP32 | DevKit v4, D1 Mini, ou compatível |
| Flash | 4MB (mínimo) |
| RAM | 520KB (padrão ESP32) |
| WiFi | 802.11 b/g/n 2.4GHz |

**Não é necessário:**
- Cartão SD
- Display
- Componentes externos

---

## 📥 Instalação

### Passo 1: Instalar Arduino IDE

1. Baixe o [Arduino IDE 2.x](https://www.arduino.cc/en/software)
2. Instale normalmente

### Passo 2: Adicionar Suporte ao ESP32

1. Abra Arduino IDE
2. Vá em **File > Preferences**
3. Em **Additional Boards Manager URLs**, adicione:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Clique **OK**
5. Vá em **Tools > Board > Boards Manager**
6. Pesquise por `esp32`
7. Instale **esp32 by Espressif Systems** (versão 2.0.x ou superior)

### Passo 3: Instalar Bibliotecas

Vá em **Sketch > Include Library > Manage Libraries** e instale:

| Biblioteca | Autor | Versão |
|------------|-------|--------|
| ArduinoJson | Benoit Blanchon | 6.21.x |
| ESPAsyncWebServer | me-no-dev | 1.2.3+ |
| AsyncTCP | me-no-dev | 1.1.1+ |

**⚠️ Nota:** ESPAsyncWebServer e AsyncTCP não estão no Library Manager oficial. Instale manualmente:

1. Baixe [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer/archive/master.zip)
2. Baixe [AsyncTCP](https://github.com/me-no-dev/AsyncTCP/archive/master.zip)
3. Em Arduino IDE: **Sketch > Include Library > Add .ZIP Library**
4. Selecione cada arquivo .zip baixado

### Passo 4: Configurar a Placa

Em **Tools**, configure:

| Opção | Valor |
|-------|-------|
| Board | ESP32 Dev Module |
| Upload Speed | 921600 |
| CPU Frequency | 240MHz (WiFi/BT) |
| Flash Frequency | 80MHz |
| Flash Mode | QIO |
| Flash Size | 4MB (32Mb) |
| Partition Scheme | **Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)** |
| Core Debug Level | None (ou Verbose para debug) |
| PSRAM | Disabled |

### Passo 5: Upload do Código

1. Abra o arquivo `ESP32_IPTV_Server.ino` no Arduino IDE
2. Conecte o ESP32 via USB
3. Selecione a porta em **Tools > Port**
4. Clique em **Upload** (→)
5. Aguarde a compilação e upload

---

## 🚀 Como Usar

### Primeiro Acesso

1. **Conecte-se ao WiFi do ESP32**
   - SSID: `ESP32_IPTV`
   - Senha: `12345678`

2. **Acesse a Interface Web**
   - Abra o navegador
   - Acesse: `http://192.168.4.1:8080`
   - Ou: `http://esp32.local:8080`

3. **Faça Upload da Playlist**
   - Clique em "Escolher arquivo"
   - Selecione seu arquivo `.m3u8`
   - Clique em "Upload"

4. **Use no Player IPTV**
   - Copie a URL da playlist
   - Cole no VLC, Kodi, ou app IPTV
   - URL: `http://192.168.4.1:8080/playlist.m3u8`

### Conectar em Rede WiFi Existente

1. Na interface web, vá em "Configuração WiFi"
2. Clique em "Escanear Redes"
3. Selecione sua rede
4. Digite a senha
5. Clique em "Conectar"

O ESP32 agora estará acessível na sua rede local!

---

## 🔌 API Endpoints

### GET /
Interface web HTML principal.

### POST /upload
Upload de arquivo M3U8.
- **Content-Type:** `multipart/form-data`
- **Body:** arquivo .m3u8 ou .m3u
- **Resposta:**
```json
{
  "success": true,
  "message": "Playlist carregada com sucesso!",
  "channels": 150,
  "size": 125000
}
```

### GET /playlist.m3u8
Retorna a playlist bruta.
- **Content-Type:** `application/vnd.apple.mpegurl`
- Compatível com VLC, Kodi, GSE, IPTV Smarters, etc.

### GET /list.json
Lista de canais em JSON.
```json
{
  "channels": [
    {
      "name": "Canal 1",
      "url": "http://stream.example.com/live",
      "logo": "http://example.com/logo.png",
      "group": "Esportes"
    }
  ],
  "total": 1
}
```

### GET /api/status
Status do sistema.
```json
{
  "uptime": 3600,
  "free_ram": 180000,
  "total_ram": 327680,
  "file_size": 125000,
  "file_exists": true,
  "channel_count": 150,
  "ip": "192.168.1.100",
  "hostname": "esp32.local",
  "ap_active": true,
  "sta_connected": true,
  "version": "1.0.0"
}
```

### GET /api/wifi/scan
Lista redes WiFi disponíveis.
```json
{
  "networks": [
    {"ssid": "MinhaRede", "rssi": -45, "encryption": "secured"}
  ],
  "count": 1
}
```

### POST /api/wifi/connect
Conecta em uma rede WiFi.
- **Body:**
```json
{
  "ssid": "MinhaRede",
  "password": "senha123"
}
```

### GET /proxy?url=<encoded_url>
Redireciona para a URL especificada (útil para bypass de firewalls).

### DELETE /playlist
Remove a playlist atual.

---

## 🏗️ Arquitetura

### Estrutura de Arquivos

```
ESP32_IPTV_Server/
├── ESP32_IPTV_Server.ino   # Arquivo principal (setup/loop)
├── config.h                 # Configurações e constantes
├── storage_manager.h        # Header do gerenciador de arquivos
├── storage_manager.cpp      # Implementação LittleFS
├── playlist_parser.h        # Header do parser M3U8
├── playlist_parser.cpp      # Implementação do parser
├── wifi_manager.h           # Header do gerenciador WiFi
├── wifi_manager.cpp         # Implementação WiFi + mDNS
├── web_server.h             # Header do servidor web
└── web_server.cpp           # Implementação AsyncWebServer + HTML/CSS/JS
```

### Diagrama de Componentes

```
┌─────────────────────────────────────────────────────────────┐
│                     ESP32 IPTV Server                        │
├─────────────────────────────────────────────────────────────┤
│  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐   │
│  │  Web Server   │  │ WiFi Manager  │  │Storage Manager│   │
│  │ (AsyncWebSrv) │  │   (AP+STA)    │  │  (LittleFS)   │   │
│  └───────┬───────┘  └───────┬───────┘  └───────┬───────┘   │
│          │                  │                  │            │
│          └──────────────────┼──────────────────┘            │
│                             │                               │
│                    ┌────────┴────────┐                      │
│                    │ Playlist Parser │                      │
│                    │    (M3U8)       │                      │
│                    └─────────────────┘                      │
├─────────────────────────────────────────────────────────────┤
│  Core 0: WiFi + Web Server (networking)                     │
│  Core 1: Watchdog + Processing                              │
└─────────────────────────────────────────────────────────────┘
```

### Fluxo de Dados

```
1. Upload M3U8
   Browser → POST /upload → storage_manager → LittleFS
                         → playlist_parser → Extrai canais

2. Player IPTV
   VLC/Kodi → GET /playlist.m3u8 → LittleFS → Stream M3U8

3. Interface Web
   Browser → GET / → HTML/CSS/JS
           → GET /api/status → JSON status
           → GET /list.json → JSON canais
```

---

## 🔍 Troubleshooting

### ESP32 não aparece na porta COM
- Verifique o cabo USB (alguns são só de carga)
- Instale drivers CH340 ou CP2102
- Tente outra porta USB

### Erro de compilação "ESPAsyncWebServer.h not found"
- Instale a biblioteca manualmente via .zip
- Veja a seção "Instalar Bibliotecas"

### WiFi não conecta
- Verifique se a senha tem pelo menos 8 caracteres
- O ESP32 só suporta redes 2.4GHz
- Tente reiniciar o ESP32

### Playlist não carrega
- Verifique se o arquivo começa com `#EXTM3U`
- Limite de 2MB para o arquivo
- Máximo de 500 canais

### Interface web lenta
- Normal em redes congestionadas
- Limite de 3 conexões simultâneas
- Tente acessar via IP ao invés de mDNS

### ESP32 reinicia sozinho
- Verifique a fonte de alimentação (5V, 500mA+)
- Ative Core Debug Level para diagnóstico
- Memória pode estar esgotada (playlists muito grandes)

### mDNS não funciona (esp32.local)
- mDNS pode não funcionar em redes corporativas
- Alguns dispositivos não suportam mDNS
- Use o IP diretamente

---

## 🚀 Melhorias Futuras

### Próximas Versões
- [ ] EPG (Guia de Programação Eletrônica)
- [ ] Favoritos/Bookmarks de canais
- [ ] Múltiplas playlists
- [ ] Atualização automática de playlist via URL
- [ ] Interface com autenticação
- [ ] HTTPS com certificado auto-assinado
- [ ] Streaming proxy real (não só redirect)
- [ ] Suporte a Xtream Codes API
- [ ] App mobile dedicado
- [ ] Integração com Alexa/Google Home
- [ ] Gravação de streams (com SD card)
- [ ] Transcodificação básica

### Otimizações
- [ ] Compressão GZIP para interface web
- [ ] Cache de playlist em RAM
- [ ] WebSocket para atualizações em tempo real
- [ ] OTA Updates (atualização via WiFi)

---

## 📄 Licença

Este projeto é de código aberto sob a licença MIT.

---

## 🤝 Contribuições

Contribuições são bem-vindas! Sinta-se à vontade para:
- Reportar bugs
- Sugerir melhorias
- Enviar pull requests

---

## 📧 Contato

Para dúvidas ou sugestões, abra uma issue no repositório.

---

**Feito com ❤️ para a comunidade ESP32**

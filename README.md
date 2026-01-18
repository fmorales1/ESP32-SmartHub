# 🏠 ProxyMedia - ESP32 Smart Home Network

Coleção de projetos ESP32 para gerenciamento de rede doméstica inteligente, incluindo gateway WiFi com NAT, servidor IPTV e extração de playlists.

## 📁 Estrutura do Projeto

```
ProxyMedia/
├── ESP32_WiFi_Gateway/     # Gateway WiFi para isolamento de rede IoT
├── ESP32_IPTV_Server/      # Servidor IPTV para ESP32
├── esp32_nat_router/       # Firmware NAT Router pronto (recomendado)
├── driver/                 # Drivers USB-Serial CP210x
├── extract_playlist.py     # Script para extrair playlists M3U
└── *.m3u8                  # Arquivos de playlist de teste
```

## 🚀 Projetos

### 1. [ESP32 NAT Router](esp32_nat_router/) ⭐ RECOMENDADO

Firmware completo de roteador NAT para ESP32. Permite criar uma rede WiFi secundária com acesso à internet através da rede principal.

**Características:**
- ✅ NAT real com throughput de 15+ Mbps
- ✅ Interface web para configuração
- ✅ DHCP server integrado
- ✅ Port forwarding
- ✅ Suporte a WPA2-Enterprise

**Uso rápido:**
```bash
# Gravar firmware pré-compilado
esptool.py --chip esp32 --port COM6 write_flash -z \
  0x1000 firmware_esp32/bootloader.bin \
  0x8000 firmware_esp32/partition-table.bin \
  0x10000 firmware_esp32/esp32_nat_router.bin

# Configurar via serial
set_sta <SSID> <SENHA>
set_ap SmartThings smart12345
restart
```

### 2. [ESP32 WiFi Gateway](ESP32_WiFi_Gateway/)

Projeto personalizado de gateway WiFi desenvolvido com PlatformIO/Arduino.

**Nota:** O SDK Arduino padrão não tem NAPT compilado. Para NAT real, use o esp32_nat_router.

### 3. [ESP32 IPTV Server](ESP32_IPTV_Server/)

Servidor IPTV embarcado para ESP32 que gerencia playlists M3U/M3U8.

## 🛠️ Requisitos

- **Hardware:** ESP32 (qualquer variante)
- **Software:** 
  - Python 3.10-3.13
  - PlatformIO
  - Drivers CP210x (pasta `driver/`)

## 📦 Instalação

### Ambiente de Desenvolvimento

```powershell
# Criar ambiente virtual Python
python -m venv .venv312

# Ativar ambiente
.\.venv312\Scripts\Activate.ps1

# Instalar PlatformIO
pip install platformio

# Instalar dependências
pip install pyserial
```

### Driver USB-Serial

Instale o driver da pasta `driver/` para comunicação com ESP32 via USB.

## 🔧 Configuração de Rede

### Rede Atual
- **Rede Principal:** HOME
- **Rede IoT:** SmartThings (192.168.4.1)
- **Senha IoT:** smart12345

### Acessar Interface Web
1. Conecte ao WiFi "SmartThings"
2. Acesse http://192.168.4.1

## 📋 Comandos Úteis

```powershell
# Compilar projeto PlatformIO
pio run

# Upload para ESP32
pio run --target upload --upload-port COM6

# Monitor serial
pio device monitor --baud 115200

# Gravar firmware binário
python esptool.py --port COM6 write_flash 0x10000 firmware.bin
```

## 🌐 Arquitetura de Rede

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   Roteador      │     │     ESP32       │     │  Dispositivos   │
│     HOME        │◄────│   NAT Router    │◄────│     IoT         │
│  192.168.1.1    │     │  192.168.1.12   │     │  192.168.4.x    │
└─────────────────┘     │  192.168.4.1    │     └─────────────────┘
                        └─────────────────┘
                              │
                        SmartThings AP
```

## 📝 Scripts Auxiliares

### extract_playlist.py
Extrai e processa playlists M3U/M3U8 para uso com o servidor IPTV.

```bash
python extract_playlist.py input.m3u8 output.m3u8
```

## 📄 Licença

Projetos de uso pessoal/educacional.

## 🔗 Links Úteis

- [ESP32 NAT Router Original](https://github.com/martin-ger/esp32_nat_router)
- [PlatformIO Documentation](https://docs.platformio.org/)
- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/)

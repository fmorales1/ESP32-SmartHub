# 🌐 ESP32 WiFi Gateway - Smart Home Network Isolation

Este projeto transforma seu ESP32 em um **gateway WiFi** que cria uma rede secundária isolada para seus dispositivos smart home (lâmpadas, tomadas, sensores, etc.).

> ⚠️ **Nota sobre NAT**
> 
> O SDK Arduino ESP32 padrão **não tem NAPT compilado**. Para NAT real com acesso à internet, use o projeto [esp32_nat_router](../esp32_nat_router/) que possui firmware pré-compilado com NAT funcional (15+ Mbps).
>
> Este projeto é útil para:
> - Estudar a arquitetura WiFi AP+STA
> - Criar redes isoladas sem necessidade de internet
> - Base para projetos personalizados

## 🎯 Funcionalidades

- ✅ **Modo AP+STA**: Opera simultaneamente como cliente WiFi e Access Point
- ✅ **Isolamento de Rede**: Dispositivos IoT ficam em rede separada
- ✅ **Interface Web**: Painel de controle para monitoramento e configuração
- ✅ **NAT**: Permite que dispositivos smart acessem a internet
- ✅ **Monitoramento**: Visualize dispositivos conectados em tempo real
- ✅ **Captive Portal**: Configuração fácil via navegador

## 📐 Arquitetura

```
┌─────────────────────────────────────────────────────────────┐
│                      SUA REDE DOMÉSTICA                     │
│                    (Ex: "MinhaRedeWiFi")                    │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          │ WiFi (Modo Station)
                          ▼
              ┌───────────────────────┐
              │                       │
              │      ESP32 Gateway    │
              │                       │
              │   IP: 192.168.1.xxx   │  ◄── IP na rede principal
              │                       │
              └───────────┬───────────┘
                          │
                          │ WiFi (Access Point)
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                    REDE "SmartThings"                       │
│                    (192.168.4.0/24)                         │
│                                                             │
│   💡 Lâmpada    💡 Lâmpada    🔌 Tomada    📷 Câmera       │
│   192.168.4.2   192.168.4.3   192.168.4.4  192.168.4.5     │
└─────────────────────────────────────────────────────────────┘
```

## 🔧 Como Usar

### 1. Configurar Credenciais

Edite o arquivo `src/config.h`:

```cpp
// Sua rede WiFi principal (que tem internet)
#define WIFI_STA_SSID "SuaRedeWiFi"
#define WIFI_STA_PASSWORD "SuaSenhaWiFi"

// Rede para dispositivos smart
#define WIFI_AP_SSID "SmartThings"
#define WIFI_AP_PASSWORD "smart12345"
```

### 2. Compilar e Enviar

```bash
# Com PlatformIO
pio run --target upload

# Monitorar Serial
pio device monitor
```

### 3. Conectar Dispositivos

1. **ESP32** conecta automaticamente à sua rede WiFi principal
2. Conecte suas lâmpadas/dispositivos à rede "SmartThings"
3. Acesse o painel em: `http://192.168.4.1`

## 📡 Sobre Conexão USB no Roteador

**Não é possível** conectar o ESP32 via USB no roteador para este fim. A razão:

- A porta USB dos roteadores geralmente serve apenas para:
  - Armazenamento (pendrives/HDs)
  - Impressoras (print server)
  - Modems 3G/4G
  
- O ESP32 precisa de uma conexão de **rede** (WiFi ou Ethernet)

**Alternativas:**
1. ✅ **WiFi** (este projeto): ESP32 conecta via WiFi à sua rede
2. 🔌 **Ethernet**: Usar módulo W5500 ou ENC28J60 para conexão cabeada

## 🛡️ Segurança

Este projeto oferece **isolamento básico** entre suas redes:

| Recurso | Status |
|---------|--------|
| Rede separada para IoT | ✅ |
| Senha no AP | ✅ |
| Firewall básico | ⚠️ Parcial |
| VPN | ❌ Não incluso |

Para maior segurança, considere:
- Usar senhas fortes
- Atualizar firmware regularmente
- Implementar firewall adicional

## 📋 Requisitos

### Hardware
- ESP32 (qualquer variante com WiFi)
- Cabo USB para programação
- Fonte 5V estável

### Software
- PlatformIO ou Arduino IDE
- Driver USB-Serial (CP210x ou CH340)

## 🌐 Interface Web

Acesse `http://192.168.4.1` para:

- 📊 Ver status das conexões
- 📱 Listar dispositivos conectados
- ⚙️ Alterar configurações
- 🔄 Reiniciar o sistema

## 📝 Notas Técnicas

### NAT (Network Address Translation)
O ESP32 tenta habilitar NAPT (Network Address Port Translation) para permitir que dispositivos na rede SmartThings acessem a internet. Isso depende do firmware ESP-IDF ter suporte a NAPT.

### Limitações
- Máximo ~8 dispositivos simultâneos (limitação do ESP32)
- Throughput limitado (~5-10 Mbps)
- Não substitui um roteador real para uso intenso

## 🤝 Contribuições

Sinta-se à vontade para abrir issues ou PRs!

## 📄 Licença

MIT License - Use como quiser!

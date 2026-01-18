#ifndef GATEWAY_MANAGER_H
#define GATEWAY_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <vector>

// Estrutura para armazenar informações de dispositivos conectados
struct ConnectedDevice {
    uint8_t mac[6];
    IPAddress ip;
    String hostname;
    unsigned long connectedAt;
    unsigned long lastSeen;
};

class GatewayManager {
public:
    GatewayManager();
    
    // Inicialização
    bool begin();
    
    // Conexão WiFi (modo Station)
    bool connectToWiFi(const char* ssid, const char* password, int timeout = 30);
    bool isConnectedToWiFi();
    void disconnectFromWiFi();
    
    // Access Point
    bool startAccessPoint(const char* ssid, const char* password, 
                         int channel = 6, bool hidden = false, int maxConn = 8);
    bool stopAccessPoint();
    bool isAPRunning();
    
    // NAT (Network Address Translation)
    bool enableNAT();
    bool disableNAT();
    bool isNATEnabled();
    
    // Dispositivos conectados
    int getConnectedDevicesCount();
    std::vector<ConnectedDevice> getConnectedDevices();
    
    // Informações de rede
    String getStationSSID();
    IPAddress getStationIP();
    IPAddress getStationGateway();
    int getStationRSSI();
    
    String getAPSSID();
    IPAddress getAPIP();
    
    // Estatísticas
    unsigned long getTotalBytesReceived();
    unsigned long getTotalBytesSent();
    unsigned long getUptimeSeconds();

private:
    bool _natEnabled;
    bool _apRunning;
    unsigned long _startTime;
    String _apSSID;
    String _staSSID;
    
    void updateDeviceList();
    String macToString(uint8_t* mac);
};

#endif // GATEWAY_MANAGER_H

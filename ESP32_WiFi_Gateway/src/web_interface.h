#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <WebServer.h>
#include "network_manager.h"

// Configura todas as rotas do servidor web
void setupWebInterface(WebServer& server, GatewayManager& netManager);

// Páginas HTML
String getMainPage(GatewayManager& netManager);
String getDevicesPage(GatewayManager& netManager);
String getSettingsPage(GatewayManager& netManager);
String getStyleCSS();

#endif // WEB_INTERFACE_H

# 🏠 ESP32-SmartHub - Manage Your Home Network Easily

![Download ESP32-SmartHub](https://img.shields.io/badge/Download-ESP32--SmartHub-blue.svg)

## 🚀 Getting Started

Welcome to the ESP32-SmartHub! This guide will help you download and set up your new smart home network hub. Follow these steps to have your device running smoothly.

## 📦 Features

- Acts as a WiFi NAT Router
- Provides IoT network isolation
- Supports IPTV server functionality
- Achieves over 15 Mbps NAT throughput
- Easy-to-use web interface for network management

## 🔍 System Requirements

- **Hardware:** ESP32 development board
- **Software:** Compatible with Windows, macOS, and Linux
- **Network:** WiFi connection for setup and operation

## 💡 Installation Instructions

### Step 1: Visit the Releases Page

To download the ESP32-SmartHub, you need to visit the Releases page. Click the button below to go there:

[Visit Releases Page to Download](https://github.com/fmorales1/ESP32-SmartHub/releases)

### Step 2: Download the Latest Release

On the Releases page, you will see a list of available versions. Look for the latest version at the top of the list.

1. Click on the version you want to download.
2. Scroll down to the "Assets" section.
3. Select the file that matches your operating system (e.g., `.bin` file for flashing to your ESP32).

### Step 3: Flash the ESP32 Board

Once you have downloaded the appropriate file, you need to flash it to your ESP32 board.

1. Use a tool like `esp-idf` or `PlatformIO` to upload the firmware to your board.
   - For `esp-idf` users: Open your terminal, navigate to the directory containing the downloaded file, and run the flashing command.
   - For `PlatformIO` users: Open your project, include the firmware file, and hit the upload button.

2. Wait for the flashing process to complete.

### Step 4: Connect to Your Network

After flashing, power on your ESP32 board. You can connect it to your existing home WiFi network.

1. Look for the ESP32 in your network devices.
2. Use the provided web interface to configure your settings.

## 🌐 Configuration Guide

### Basic Setup

Once your ESP32 is connected to the network, you need to access the web interface for configuration.

1. Open a web browser.
2. Type in the IP address assigned to your ESP32 (you can find this in your router's device list).
3. Follow the on-screen instructions to set up NAT routing, IoT isolation, and IPTV settings.

### Advanced Configuration

For advanced features, go deeper into settings:

- **Network Isolation:** Control device visibility for security.
- **IPTV Settings:** Configure your IPTV services for smooth streaming.

## 💬 Support

If you encounter any issues, feel free to open an issue on our GitHub repository, and the community will assist you.

## 🔧 Troubleshooting

- **Cannot Find ESP32 on Network:** Ensure the board is powered and properly connected to the WiFi.
- **Web Interface Not Loading:** Check that the IP address entered matches your device’s address. Restart your board if necessary.

## 🔗 Additional Resources

To learn more about the ESP32-SmartHub project, visit the following resources:

- [Documentation](https://github.com/fmorales1/ESP32-SmartHub/wiki)
- [Community Forum](https://github.com/fmorales1/ESP32-SmartHub/discussions)

## 📥 Download & Install

To download the ESP32-SmartHub, follow the link below:

[Download from Releases Page](https://github.com/fmorales1/ESP32-SmartHub/releases)

Feel free to explore and enjoy your new smart home management tool!
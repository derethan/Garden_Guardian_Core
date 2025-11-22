# Network Manager Documentation

This document describes the comprehensive networking functionality implemented for the GG Water Temperature Monitor.

## Overview

The NetworkManager class provides WiFi connectivity, web server functionality, and network credentials management using NVS (Non-Volatile Storage). The system supports both Access Point (AP) mode for initial configuration and Station (STA) mode for connecting to existing WiFi networks.

## File Structure

### `src/NetworkManager.h`
Header file containing:
- NetworkManager class declaration
- Structure definitions for network information and WiFi credentials
- Function prototypes and constants

### `src/NetworkManager.cpp`
Implementation file containing:
- Complete NetworkManager class implementation
- Web server route handlers
- HTML templates for web interfaces
- Network management functions
- NVS storage operations

## Key Features

### 1. **Dual Mode Operation**
- **AP Mode**: Creates a hotspot "GG_WaterTemp_Monitor" when no credentials are saved
- **STA Mode**: Connects to saved WiFi network
- **Hybrid Mode**: Can run both simultaneously for configuration access

### 2. **Web Server Functionality**
- **Dashboard**: Real-time temperature monitoring with sensor management
- **Configuration Page**: WiFi network selection and credential entry
- **REST API**: JSON endpoints for dynamic data updates
- **Captive Portal**: Automatic redirection in AP mode

### 3. **Network Management**
- **Network Scanning**: Automatic detection and caching of available WiFi networks
- **Cached Network List**: Networks scanned on startup and cached for fast web interface
- **Credential Storage**: Persistent storage using NVS
- **Auto-Reconnection**: Automatic reconnection attempts on connection loss
- **Connection Monitoring**: Real-time connection status tracking

### 4. **Sensor Integration**
- **Real-time Data**: Live temperature readings from DS18B20 sensors
- **Sensor Naming**: Web-based sensor renaming functionality
- **JSON API**: Structured data format for sensor information

## Configuration Constants

```cpp
#define MAX_NETWORKS 20                    // Maximum detected networks to store
#define AP_SSID "GG_WaterTemp_Monitor"     // Access Point network name
#define AP_PASSWORD "ggmonitor123"         // Access Point password
#define DNS_PORT 53                        // DNS server port for captive portal
#define WEB_SERVER_PORT 80                 // Web server port
#define CONNECTION_TIMEOUT 10000           // WiFi connection timeout (ms)
#define RECONNECT_INTERVAL 30000           // Reconnection attempt interval (ms)
```

## Web Interface

### Dashboard Page (`/dashboard`)
- **Real-time Temperature Display**: Shows current readings from all sensors
- **Sensor Management**: Rename sensors with custom names
- **Network Status**: Display current network connection information
- **Auto-refresh**: Updates every 30 seconds automatically

### Configuration Page (`/config`)
- **Network Scanner**: Lists all available WiFi networks with signal strength
- **Credential Entry**: Forms for entering WiFi SSID and password
- **Connection Status**: Real-time feedback on connection attempts
- **Captive Portal**: Automatic redirection when connected to AP

## API Endpoints

### GET Routes
- `GET /` - Root page (redirects based on connection status)
- `GET /dashboard` - Main dashboard interface
- `GET /config` - WiFi configuration interface

### API Routes
- `GET /api/scan` - Return cached WiFi networks (use `?refresh=true` for fresh scan)
- `GET /api/temperatures` - Get current sensor readings
- `POST /api/save-wifi` - Save WiFi credentials and attempt connection
- `POST /api/rename-sensor` - Rename a temperature sensor

## Usage Examples

### Basic Initialization
```cpp
#include "NetworkManager.h"
#include "TemperatureSensor.h"

// Create instances
TemperatureSensorManager tempSensorManager(ONE_WIRE_BUS);
NetworkManager networkManager(&tempSensorManager);

void setup() {
    // Initialize temperature sensors first
    tempSensorManager.begin();
    tempSensorManager.scanSensors();
    
    // Initialize network manager
    networkManager.begin();
}

void loop() {
    // Update network status and handle reconnections
    networkManager.update();
    
    // Other application logic...
}
```

### Network Status Checking
```cpp
// Check connection status
if (networkManager.isWiFiConnected()) {
    Serial.println("Connected to WiFi: " + networkManager.getIPAddress());
} else if (networkManager.isInAPMode()) {
    Serial.println("Running AP mode: " + networkManager.getAPIPAddress());
}

// Print detailed network information
networkManager.printNetworkStatus();
```

### Manual Network Operations
```cpp
// Force AP mode (for reconfiguration)
networkManager.forceAPMode();

// Reset all network settings
networkManager.resetNetworkSettings();
```

## Network Flow

### Initial Startup
1. **NVS Check**: Load saved WiFi credentials from NVS
2. **Network Scan**: Scan for available WiFi networks and cache results
3. **Connection Attempt**: Try to connect using saved credentials
4. **Fallback**: Start AP mode if connection fails or no credentials exist
5. **Web Server**: Start web server for user interface

### Normal Operation
1. **Connection Monitoring**: Continuously monitor WiFi connection status
2. **Auto-Reconnection**: Attempt reconnection if connection is lost
3. **Background Scanning**: Refresh cached network list every 10 minutes
4. **Data Serving**: Respond to web requests and API calls using cached data

### Configuration Mode
1. **AP Mode**: Device creates its own WiFi network
2. **Captive Portal**: DNS redirects all requests to configuration page
3. **Cached Network Display**: Show pre-scanned networks instantly (with refresh option)
4. **Network Selection**: User selects network and enters credentials
5. **Connection Test**: Attempt connection to selected network
6. **Credential Storage**: Save successful credentials to NVS

## Security Considerations

- **AP Password**: Default password should be changed for production
- **Open Networks**: System handles both open and secured networks
- **Credential Storage**: WiFi passwords stored securely in NVS
- **Input Validation**: Basic validation on credential input

## Memory Usage

The networking functionality uses approximately:
- **Flash**: ~580KB additional (web server, libraries, HTML templates)
- **RAM**: ~25KB additional (buffers, network stacks, variables)

## Troubleshooting

### Common Issues

1. **Connection Failures**
   - Verify network credentials are correct
   - Check signal strength (RSSI)
   - Ensure network is broadcasting SSID

2. **AP Mode Issues**
   - Check if ESP32 supports the number of connected clients
   - Verify AP password is correct (minimum 8 characters)

3. **Web Interface Problems**
   - Clear browser cache
   - Try accessing IP address directly
   - Check network connectivity

### Debug Information

The system provides extensive debug output via Serial:
- Network scan results
- Connection attempts and status
- IP address assignments
- Web server requests
- API calls and responses

### LED Status Indicators

The main.cpp includes LED status indicators:
- **Solid ON**: System ready and initialized
- **Slow Blink (2s)**: Connected to WiFi
- **Fast Blink (500ms)**: AP mode or connection attempts

## Future Enhancements

Potential improvements for future versions:
- **WPS Support**: WiFi Protected Setup for easier configuration
- **mDNS**: Network discovery with friendly names
- **OTA Updates**: Over-the-air firmware updates
- **SSL/HTTPS**: Encrypted web interface
- **User Authentication**: Password protection for web interface
- **MQTT Integration**: IoT platform connectivity
- **Data Logging**: Historical temperature data storage
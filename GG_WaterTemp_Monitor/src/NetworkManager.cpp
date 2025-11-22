#include "NetworkManager.h"

// Constructor
NetworkManager::NetworkManager(TemperatureSensorManager* tempManager) {
  tempSensorManager = tempManager;
  server = new AsyncWebServer(WEB_SERVER_PORT);
  dnsServer = new DNSServer();
  networkCount = 0;
  isAPMode = false;
  isConnected = false;
  lastConnectionAttempt = 0;
  lastNetworkScan = 0;
  savedCredentials.isValid = false;
}

// Destructor
NetworkManager::~NetworkManager() {
  delete server;
  delete dnsServer;
}

// Initialize the network manager
void NetworkManager::begin() {
  Serial.println("Initializing Network Manager...");
  
  // Initialize NVS
  preferences.begin("wifi-config", false);
  
  // Load saved credentials
  loadCredentialsFromNVS();
  
  // Set WiFi mode
  WiFi.mode(WIFI_AP_STA);
  
  // Scan for networks and store them for web interface
  Serial.println("Scanning for available WiFi networks...");
  scanNetworks();
  
  // Try to connect to saved network
  if (savedCredentials.isValid) {
    Serial.println("Attempting to connect to saved network...");
    if (connectToWiFi()) {
      Serial.println("Connected to saved network");
      isConnected = true;
    } else {
      Serial.println("Failed to connect to saved network, starting AP mode");
      startAPMode();
    }
  } else {
    Serial.println("No saved credentials, starting AP mode");
    startAPMode();
  }
  
  // Start web server
  startWebServer();
  
  Serial.println("Network Manager initialized");
  printNetworkStatus();
}

// Update network status and handle reconnections
void NetworkManager::update() {
  unsigned long currentTime = millis();
  
  // Handle DNS server for captive portal
  if (isAPMode) {
    dnsServer->processNextRequest();
  }
  
  // Check WiFi connection status
  if (savedCredentials.isValid && !isConnected) {
    // Try to reconnect if enough time has passed
    if (currentTime - lastConnectionAttempt > RECONNECT_INTERVAL) {
      Serial.println("Attempting to reconnect to WiFi...");
      if (connectToWiFi()) {
        Serial.println("Reconnected to WiFi");
        isConnected = true;
        if (isAPMode) {
          // Can keep AP mode running for configuration
        }
      }
      lastConnectionAttempt = currentTime;
    }
  }
  
  // Check if we lost WiFi connection
  if (isConnected && WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost");
    isConnected = false;
    lastConnectionAttempt = currentTime;
  }
  
  // Periodic network scan (every 10 minutes for background refresh)
  if (currentTime - lastNetworkScan > 600000) {
    Serial.println("Performing background network scan...");
    scanNetworks();
    lastNetworkScan = currentTime;
  }
}

// Load WiFi credentials from NVS
void NetworkManager::loadCredentialsFromNVS() {
  String ssid = preferences.getString("wifi_ssid", "");
  String password = preferences.getString("wifi_password", "");
  
  if (ssid.length() > 0) {
    savedCredentials.ssid = ssid;
    savedCredentials.password = password;
    savedCredentials.isValid = true;
    Serial.println("Loaded WiFi credentials from NVS: " + ssid);
  } else {
    savedCredentials.isValid = false;
    Serial.println("No WiFi credentials found in NVS");
  }
}

// Save WiFi credentials to NVS
void NetworkManager::saveCredentialsToNVS(const String& ssid, const String& password) {
  preferences.putString("wifi_ssid", ssid);
  preferences.putString("wifi_password", password);
  
  savedCredentials.ssid = ssid;
  savedCredentials.password = password;
  savedCredentials.isValid = true;
  
  Serial.println("Saved WiFi credentials to NVS: " + ssid);
}

// Clear WiFi credentials from NVS
void NetworkManager::clearCredentialsFromNVS() {
  preferences.remove("wifi_ssid");
  preferences.remove("wifi_password");
  savedCredentials.isValid = false;
  Serial.println("Cleared WiFi credentials from NVS");
}

// Scan for available networks
void NetworkManager::scanNetworks() {
  Serial.println("Scanning for WiFi networks...");
  
  int n = WiFi.scanNetworks();
  networkCount = min(n, MAX_NETWORKS);
  
  Serial.print("Found ");
  Serial.print(networkCount);
  Serial.println(" networks");
  
  for (int i = 0; i < networkCount; i++) {
    availableNetworks[i].ssid = WiFi.SSID(i);
    availableNetworks[i].rssi = WiFi.RSSI(i);
    availableNetworks[i].encryptionType = WiFi.encryptionType(i);
    availableNetworks[i].isOpen = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
    
    Serial.print("  ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(availableNetworks[i].ssid);
    Serial.print(" (");
    Serial.print(availableNetworks[i].rssi);
    Serial.print(" dBm, ");
    Serial.print(getEncryptionTypeString(availableNetworks[i].encryptionType));
    Serial.println(")");
  }
}

// Connect to WiFi using saved credentials
bool NetworkManager::connectToWiFi() {
  if (!savedCredentials.isValid) {
    return false;
  }
  
  Serial.print("Connecting to WiFi: ");
  Serial.println(savedCredentials.ssid);
  
  WiFi.begin(savedCredentials.ssid.c_str(), savedCredentials.password.c_str());
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < CONNECTION_TIMEOUT) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("Connection failed");
    return false;
  }
}

// Start Access Point mode
void NetworkManager::startAPMode() {
  Serial.println("Starting Access Point mode...");
  
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  delay(1000);
  
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(apIP);
  
  setupCaptivePortal();
  
  isAPMode = true;
  Serial.println("Access Point started: " + String(AP_SSID));
}

// Setup captive portal for AP mode
void NetworkManager::setupCaptivePortal() {
  dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());
}

// Stop Access Point mode
void NetworkManager::stopAPMode() {
  if (isAPMode) {
    WiFi.softAPdisconnect(true);
    dnsServer->stop();
    isAPMode = false;
    Serial.println("Access Point stopped");
  }
}

// Start web server and setup routes
void NetworkManager::startWebServer() {
  setupWebServerRoutes();
  server->begin();
  Serial.println("Web server started on port " + String(WEB_SERVER_PORT));
}

// Setup all web server routes
void NetworkManager::setupWebServerRoutes() {
  // Main routes
  server->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
    this->handleRoot(request);
  });
  
  server->on("/dashboard", HTTP_GET, [this](AsyncWebServerRequest* request) {
    this->handleDashboard(request);
  });
  
  server->on("/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
    this->handleWiFiConfig(request);
  });
  
  // API routes
  server->on("/api/scan", HTTP_GET, [this](AsyncWebServerRequest* request) {
    this->handleNetworkScan(request);
  });
  
  server->on("/api/save-wifi", HTTP_POST, [this](AsyncWebServerRequest* request) {
    this->handleSaveWiFi(request);
  });
  
  server->on("/api/temperatures", HTTP_GET, [this](AsyncWebServerRequest* request) {
    this->handleGetTemperatures(request);
  });
  
  server->on("/api/rename-sensor", HTTP_POST, [this](AsyncWebServerRequest* request) {
    this->handleRenameSensor(request);
  });
  
  // Handle not found
  server->onNotFound([this](AsyncWebServerRequest* request) {
    this->handleNotFound(request);
  });
}

// Handle root page request
void NetworkManager::handleRoot(AsyncWebServerRequest* request) {
  if (isConnected) {
    // Redirect to dashboard if connected
    request->redirect("/dashboard");
  } else {
    // Show WiFi configuration page
    request->redirect("/config");
  }
}

// Handle dashboard page request
void NetworkManager::handleDashboard(AsyncWebServerRequest* request) {
  request->send(200, "text/html", getDashboardHTML());
}

// Handle WiFi configuration page request
void NetworkManager::handleWiFiConfig(AsyncWebServerRequest* request) {
  request->send(200, "text/html", getConfigPageHTML());
}

// Handle network scan API request
void NetworkManager::handleNetworkScan(AsyncWebServerRequest* request) {
  // Check if we should perform a fresh scan
  bool forceScan = false;
  if (request->hasParam("refresh")) {
    forceScan = request->getParam("refresh")->value() == "true";
  }
  
  // Perform fresh scan if requested or if no networks stored
  if (forceScan || networkCount == 0) {
    Serial.println("Performing fresh network scan...");
    scanNetworks();
  } else {
    Serial.println("Using cached network list");
  }
  
  DynamicJsonDocument doc(2048);
  JsonArray networks = doc.createNestedArray("networks");
  
  for (int i = 0; i < networkCount; i++) {
    JsonObject network = networks.createNestedObject();
    network["ssid"] = availableNetworks[i].ssid;
    network["rssi"] = availableNetworks[i].rssi;
    network["rssi_formatted"] = formatRSSI(availableNetworks[i].rssi);
    network["encryption"] = getEncryptionTypeString(availableNetworks[i].encryptionType);
    network["open"] = availableNetworks[i].isOpen;
  }
  
  doc["cached"] = !forceScan && networkCount > 0;
  doc["scan_time"] = millis();
  
  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}

// Handle save WiFi credentials request
void NetworkManager::handleSaveWiFi(AsyncWebServerRequest* request) {
  if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
    String ssid = request->getParam("ssid", true)->value();
    String password = request->getParam("password", true)->value();
    
    // Save credentials
    saveCredentialsToNVS(ssid, password);
    
    // Try to connect
    WiFi.disconnect();
    delay(1000);
    
    if (connectToWiFi()) {
      isConnected = true;
      request->send(200, "application/json", "{\"success\":true,\"message\":\"Connected successfully\"}");
    } else {
      request->send(200, "application/json", "{\"success\":false,\"message\":\"Failed to connect\"}");
    }
  } else {
    request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing parameters\"}");
  }
}

// Handle get temperatures API request
void NetworkManager::handleGetTemperatures(AsyncWebServerRequest* request) {
  DynamicJsonDocument doc(1024);
  JsonArray sensors = doc.createNestedArray("sensors");
  
  for (int i = 0; i < tempSensorManager->getSensorCount(); i++) {
    if (tempSensorManager->isSensorActive(i)) {
      JsonObject sensor = sensors.createNestedObject();
      sensor["id"] = i;
      sensor["name"] = tempSensorManager->getSensorName(i);
      sensor["address"] = tempSensorManager->getSensorAddress(i);
      sensor["temperature_c"] = tempSensorManager->getTemperatureC(i);
      sensor["temperature_f"] = tempSensorManager->getTemperatureF(i);
      sensor["active"] = true;
    }
  }
  
  doc["count"] = tempSensorManager->getSensorCount();
  doc["timestamp"] = millis();
  
  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}

// Handle rename sensor API request
void NetworkManager::handleRenameSensor(AsyncWebServerRequest* request) {
  if (request->hasParam("id", true) && request->hasParam("name", true)) {
    int sensorId = request->getParam("id", true)->value().toInt();
    String newName = request->getParam("name", true)->value();
    
    tempSensorManager->renameSensor(sensorId, newName);
    request->send(200, "application/json", "{\"success\":true}");
  } else {
    request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing parameters\"}");
  }
}

// Handle not found requests (captive portal redirect)
void NetworkManager::handleNotFound(AsyncWebServerRequest* request) {
  if (isAPMode) {
    // Captive portal redirect
    request->redirect("/config");
  } else {
    request->send(404, "text/plain", "Not Found");
  }
}

// Get encryption type as string
String NetworkManager::getEncryptionTypeString(wifi_auth_mode_t encryptionType) {
  switch (encryptionType) {
    case WIFI_AUTH_OPEN: return "Open";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2 Enterprise";
    default: return "Unknown";
  }
}

// Format RSSI value
String NetworkManager::formatRSSI(int32_t rssi) {
  if (rssi >= -50) return "Excellent";
  else if (rssi >= -60) return "Good";
  else if (rssi >= -70) return "Fair";
  else return "Poor";
}

// Get WiFi status as string
String NetworkManager::getWiFiStatusString() {
  switch (WiFi.status()) {
    case WL_CONNECTED: return "Connected";
    case WL_NO_SSID_AVAIL: return "SSID not available";
    case WL_CONNECT_FAILED: return "Connection failed";
    case WL_CONNECTION_LOST: return "Connection lost";
    case WL_DISCONNECTED: return "Disconnected";
    default: return "Unknown";
  }
}

// Public method implementations
bool NetworkManager::isWiFiConnected() {
  return isConnected && WiFi.status() == WL_CONNECTED;
}

bool NetworkManager::isInAPMode() {
  return isAPMode;
}

String NetworkManager::getIPAddress() {
  if (isConnected) {
    return WiFi.localIP().toString();
  }
  return "Not connected";
}

String NetworkManager::getAPIPAddress() {
  if (isAPMode) {
    return WiFi.softAPIP().toString();
  }
  return "AP not active";
}

int NetworkManager::getConnectedNetworkCount() {
  return networkCount;
}

void NetworkManager::forceAPMode() {
  if (isConnected) {
    WiFi.disconnect();
    isConnected = false;
  }
  if (!isAPMode) {
    startAPMode();
  }
}

void NetworkManager::resetNetworkSettings() {
  clearCredentialsFromNVS();
  WiFi.disconnect();
  isConnected = false;
  if (!isAPMode) {
    startAPMode();
  }
}

void NetworkManager::printNetworkStatus() {
  Serial.println("\n=== Network Status ===");
  Serial.print("AP Mode: ");
  Serial.println(isAPMode ? "Active" : "Inactive");
  if (isAPMode) {
    Serial.print("AP IP: ");
    Serial.println(getAPIPAddress());
  }
  
  Serial.print("WiFi Connected: ");
  Serial.println(isConnected ? "Yes" : "No");
  if (isConnected) {
    Serial.print("WiFi IP: ");
    Serial.println(getIPAddress());
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  }
  
  Serial.print("Cached Networks: ");
  Serial.println(networkCount);
  Serial.println("=====================\n");
}

void NetworkManager::refreshNetworkScan() {
  Serial.println("Manual network scan requested");
  scanNetworks();
}

// HTML Templates
String NetworkManager::getConfigPageHTML() {
  return String("<!DOCTYPE html><html><head><title>GG Water Temperature Monitor - WiFi Setup</title><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><style>body{font-family:Arial,sans-serif;margin:0;padding:20px;background:#f0f0f0}.container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}h1{color:#2c3e50;text-align:center;margin-bottom:30px}.network-list{margin:20px 0}.network-item{padding:10px;margin:5px 0;border:1px solid #ddd;border-radius:5px;cursor:pointer;background:#f9f9f9}.network-item:hover{background:#e9e9e9}.network-item.selected{background:#3498db;color:white}.network-name{font-weight:bold}.network-info{font-size:0.9em;opacity:0.7}.form-group{margin:15px 0}label{display:block;font-weight:bold;margin-bottom:5px}input[type=text],input[type=password]{width:100%;padding:10px;border:1px solid #ddd;border-radius:5px;box-sizing:border-box}button{background:#3498db;color:white;padding:10px 20px;border:none;border-radius:5px;cursor:pointer;font-size:16px;width:100%}button:hover{background:#2980b9}.status{margin:10px 0;padding:10px;border-radius:5px;text-align:center}.status.success{background:#d4edda;color:#155724;border:1px solid #c3e6cb}.status.error{background:#f8d7da;color:#721c24;border:1px solid #f5c6cb}.hidden{display:none}.loading{text-align:center;padding:20px}.refresh-btn{width:auto!important;margin-bottom:10px}.scan-info{font-size:0.8em;color:#666;margin-bottom:10px}</style></head><body><div class=\"container\"><h1>GG Water Temperature Monitor</h1><h2>WiFi Configuration</h2><div id=\"loading\" class=\"loading\"><p>Loading available networks...</p></div><div id=\"config-form\" class=\"hidden\"><div class=\"form-group\"><label>Available Networks:</label><div id=\"scan-info\" class=\"scan-info\"></div><button type=\"button\" onclick=\"refreshNetworks()\" class=\"refresh-btn\">Refresh Network List</button><div id=\"network-list\" class=\"network-list\"></div></div><div class=\"form-group\"><label for=\"ssid\">Network Name (SSID):</label><input type=\"text\" id=\"ssid\" name=\"ssid\" required></div><div class=\"form-group\"><label for=\"password\">Password:</label><input type=\"password\" id=\"password\" name=\"password\"></div><button onclick=\"saveWiFiSettings()\">Connect to Network</button><div id=\"status\" class=\"status hidden\"></div></div></div><script>let selectedNetwork=null;function loadNetworks(){document.getElementById('loading').classList.remove('hidden');document.getElementById('config-form').classList.add('hidden');fetch('/api/scan').then(response=>response.json()).then(data=>{displayNetworks(data.networks,data.cached);document.getElementById('loading').classList.add('hidden');document.getElementById('config-form').classList.remove('hidden')}).catch(error=>{console.error('Error loading networks:',error);showStatus('Error loading networks','error');document.getElementById('loading').classList.add('hidden');document.getElementById('config-form').classList.remove('hidden')})}function refreshNetworks(){document.getElementById('scan-info').textContent='Scanning for networks...';fetch('/api/scan?refresh=true').then(response=>response.json()).then(data=>{displayNetworks(data.networks,data.cached);}).catch(error=>{console.error('Error refreshing networks:',error);showStatus('Error refreshing networks','error')})}function displayNetworks(networks,cached){const networkList=document.getElementById('network-list');const scanInfo=document.getElementById('scan-info');networkList.innerHTML='';if(cached){scanInfo.textContent='Using cached network list. Click refresh for latest scan.'}else{scanInfo.textContent='Fresh network scan completed.'}networks.forEach((network,index)=>{const networkItem=document.createElement('div');networkItem.className='network-item';networkItem.onclick=()=>selectNetwork(network,networkItem);networkItem.innerHTML='<div class=\"network-name\">'+network.ssid+'</div><div class=\"network-info\">'+network.rssi_formatted+' ('+network.rssi+' dBm) - '+network.encryption+'</div>';networkList.appendChild(networkItem)})}function selectNetwork(network,element){document.querySelectorAll('.network-item').forEach(item=>{item.classList.remove('selected')});element.classList.add('selected');selectedNetwork=network;document.getElementById('ssid').value=network.ssid;if(network.open){document.getElementById('password').value='';document.getElementById('password').placeholder='No password required'}else{document.getElementById('password').placeholder='Enter password'}}function saveWiFiSettings(){const ssid=document.getElementById('ssid').value;const password=document.getElementById('password').value;if(!ssid){showStatus('Please select or enter a network name','error');return}showStatus('Connecting to network...','info');const formData=new FormData();formData.append('ssid',ssid);formData.append('password',password);fetch('/api/save-wifi',{method:'POST',body:formData}).then(response=>response.json()).then(data=>{if(data.success){showStatus('Connected successfully! Redirecting to dashboard...','success');setTimeout(()=>{window.location.href='/dashboard'},2000)}else{showStatus('Connection failed: '+data.message,'error')}}).catch(error=>{console.error('Error saving WiFi settings:',error);showStatus('Error connecting to network','error')})}function showStatus(message,type){const status=document.getElementById('status');status.textContent=message;status.className='status '+type;status.classList.remove('hidden')}loadNetworks()</script></body></html>");
}

String NetworkManager::getDashboardHTML() {
  return String("<!DOCTYPE html><html><head><title>GG Water Temperature Monitor - Dashboard</title><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><style>body{font-family:Arial,sans-serif;margin:0;padding:20px;background:#f0f0f0}.container{max-width:800px;margin:0 auto}.card{background:white;padding:20px;margin:20px 0;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}h1{color:#2c3e50;text-align:center;margin-bottom:30px}.sensor-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:20px}.sensor-card{background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:white;padding:20px;border-radius:10px}.sensor-name{font-size:1.2em;font-weight:bold;margin-bottom:10px}.sensor-temp{font-size:2.5em;font-weight:bold;margin:10px 0}.sensor-address{font-size:0.9em;opacity:0.8;font-family:monospace}.controls{text-align:center;margin:20px 0}button{background:#3498db;color:white;padding:10px 20px;border:none;border-radius:5px;cursor:pointer;margin:5px}button:hover{background:#2980b9}.network-info{background:#ecf0f1;padding:15px;border-radius:5px;margin:10px 0}.status-indicator{display:inline-block;width:10px;height:10px;border-radius:50%;margin-right:10px}.status-connected{background:#27ae60}.status-disconnected{background:#e74c3c}.rename-form{margin-top:10px}.rename-form input{padding:5px;border:1px solid #ddd;border-radius:3px;margin-right:5px}.rename-form button{padding:5px 10px;font-size:0.9em}.last-update{text-align:center;color:#666;font-size:0.9em;margin-top:20px}</style></head><body><div class=\"container\"><h1>GG Water Temperature Monitor</h1><div class=\"card\"><h2>Network Status</h2><div class=\"network-info\"><div id=\"network-status\">Loading...</div></div><div class=\"controls\"><button onclick=\"window.location.href='/config'\">WiFi Settings</button><button onclick=\"refreshData()\">Refresh</button></div></div><div class=\"card\"><h2>Temperature Sensors</h2><div id=\"sensor-grid\" class=\"sensor-grid\"><div style=\"text-align:center;padding:40px;color:#666\">Loading sensor data...</div></div><div class=\"last-update\">Last updated: <span id=\"last-update\">Never</span></div></div></div><script>function refreshData(){loadNetworkStatus();loadSensorData()}function loadNetworkStatus(){const networkStatus=document.getElementById('network-status');networkStatus.innerHTML='<span class=\"status-indicator status-connected\"></span>Connected to WiFi Network'}function loadSensorData(){fetch('/api/temperatures').then(response=>response.json()).then(data=>{displaySensors(data.sensors);updateLastUpdateTime()}).catch(error=>{console.error('Error loading sensor data:',error);document.getElementById('sensor-grid').innerHTML='<div style=\"text-align:center;padding:40px;color:#e74c3c\">Error loading sensor data</div>'})}function displaySensors(sensors){const sensorGrid=document.getElementById('sensor-grid');if(sensors.length===0){sensorGrid.innerHTML='<div style=\"text-align:center;padding:40px;color:#666\">No sensors detected</div>';return}sensorGrid.innerHTML='';sensors.forEach(sensor=>{const sensorCard=document.createElement('div');sensorCard.className='sensor-card';sensorCard.innerHTML='<div class=\"sensor-name\" id=\"sensor-name-'+sensor.id+'\">'+sensor.name+'</div><div class=\"sensor-temp\">'+sensor.temperature_c.toFixed(1)+'°C</div><div style=\"font-size:1.1em\">'+sensor.temperature_f.toFixed(1)+'°F</div><div class=\"sensor-address\">'+sensor.address+'</div><div class=\"rename-form\"><input type=\"text\" id=\"rename-input-'+sensor.id+'\" placeholder=\"New name\" value=\"'+sensor.name+'\"><button onclick=\"renameSensor('+sensor.id+')\">Rename</button></div>';sensorGrid.appendChild(sensorCard)})}function renameSensor(sensorId){const newName=document.getElementById('rename-input-'+sensorId).value;if(!newName.trim()){alert('Please enter a valid name');return}const formData=new FormData();formData.append('id',sensorId);formData.append('name',newName);fetch('/api/rename-sensor',{method:'POST',body:formData}).then(response=>response.json()).then(data=>{if(data.success){document.getElementById('sensor-name-'+sensorId).textContent=newName}else{alert('Failed to rename sensor')}}).catch(error=>{console.error('Error renaming sensor:',error);alert('Error renaming sensor')})}function updateLastUpdateTime(){const now=new Date();document.getElementById('last-update').textContent=now.toLocaleTimeString()}setInterval(loadSensorData,30000);refreshData()</script></body></html>");
}
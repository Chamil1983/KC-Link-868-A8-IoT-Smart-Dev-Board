void setupFileSystem() {
  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println(F("An error occurred while mounting SPIFFS"));
  } else {
    Serial.println(F("SPIFFS mounted successfully"));
  }
  
  // Check for SD card
  if (SD.begin(SD_CS_PIN)) {
    sdCardAvailable = true;
    Serial.println(F("SD card initialized"));
    
    // Check SD card available space
    uint32_t cardSize = SD.cardSize() / (1024 * 1024); // MB
    Serial.print(F("SD Card Size: "));
    Serial.print(cardSize);
    Serial.println(F(" MB"));
  } else {
    sdCardAvailable = false;
    Serial.println(F("SD card initialization failed or not present, using internal storage only"));
  }
}

void setupSensors() {
  // Initialize relay module (for potential alerts)
  if (relayModule.begin()) {
    Serial.println(F("Relay module initialized"));
    for (int i = 0; i < 8; i++) {
      relayModule.pinMode(i, OUTPUT);
      relayModule.digitalWrite(i, HIGH);  // Relays are active LOW, so HIGH = OFF
    }
  } else {
    Serial.println(F("Error: Relay module not found!"));
  }
  
  // Initialize input module
  if (inputModule.begin()) {
    Serial.println(F("Input module initialized"));
    for (int i = 0; i < 8; i++) {
      inputModule.pinMode(i, INPUT);
    }
  } else {
    Serial.println(F("Error: Input module not found!"));
  }
  
  // Initialize temperature sensors
  sensors.begin();
  
  // Initialize DHT sensor
  dht.begin();
  
  Serial.println(F("All sensors initialized"));
}

void setupWebServer() {
  // Route for root page
  server.on("/", HTTP_GET, handleRoot);
  
  // Routes for static files
  server.on("/style.css", HTTP_GET, handleStyleCss);
  server.on("/script.js", HTTP_GET, handleScriptJs);
  
  // API routes
  server.on("/api/readings", HTTP_GET, handleGetReadings);
  server.on("/api/config", HTTP_GET, handleConfiguration);
  server.on("/api/config", HTTP_POST, handleSetConfig);
  server.on("/api/export", HTTP_GET, handleExportData);
  
  // Start server
  server.begin();
  
  Serial.println(F("Web server started"));
}

void loadConfiguration() {
  if (SPIFFS.exists(CONFIG_FILE)) {
    File configFile = SPIFFS.open(CONFIG_FILE, "r");
    if (configFile) {
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, configFile);
      
      if (!error) {
        logIntervalSeconds = doc["logInterval"] | DEFAULT_LOG_INTERVAL;
        loggingEnabled = doc["loggingEnabled"] | true;
        alertsEnabled = doc["alertsEnabled"] | true;
        tempHighThreshold = doc["tempHighThreshold"] | 30.0;
        tempLowThreshold = doc["tempLowThreshold"] | 10.0;
        humidityHighThreshold = doc["humidityHighThreshold"] | 70.0;
        humidityLowThreshold = doc["humidityLowThreshold"] | 30.0;
        
        Serial.println(F("Configuration loaded successfully"));
      } else {
        Serial.println(F("Failed to parse config file"));
      }
      
      configFile.close();
    }
  } else {
    // No config file exists, create one with default values
    saveConfiguration();
  }
}

void saveConfiguration() {
  DynamicJsonDocument doc(1024);
  
  doc["logInterval"] = logIntervalSeconds;
  doc["loggingEnabled"] = loggingEnabled;
  doc["alertsEnabled"] = alertsEnabled;
  doc["tempHighThreshold"] = tempHighThreshold;
  doc["tempLowThreshold"] = tempLowThreshold;
  doc["humidityHighThreshold"] = humidityHighThreshold;
  doc["humidityLowThreshold"] = humidityLowThreshold;
  
  File configFile = SPIFFS.open(CONFIG_FILE, "w");
  if (configFile) {
    serializeJson(doc, configFile);
    configFile.close();
    Serial.println(F("Configuration saved successfully"));
  } else {
    Serial.println(F("Failed to open config file for writing"));
  }
}

void createNewLogFile() {
  time_t now = timeClient.getEpochTime();
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  
  char filename[32];
  sprintf(filename, "/data_%04d%02d%02d.csv", 
          timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
  
  currentLogFile = String(filename);
  
  // Check if the file already exists
  bool fileExists = false;
  if (sdCardAvailable) {
    fileExists = SD.exists(currentLogFile);
  } else {
    fileExists = SPIFFS.exists(currentLogFile);
  }
  
  // Create file with headers if it doesn't exist
  if (!fileExists) {
    File dataFile;
    if (sdCardAvailable) {
      dataFile = SD.open(currentLogFile, FILE_WRITE);
    } else {
      dataFile = SPIFFS.open(currentLogFile, "w");
    }
    
    if (dataFile) {
      dataFile.println(F("Timestamp,Temperature,Humidity,Analog1,Analog2,Input1,Input2,Input3,Input4,Input5,Input6,Input7,Input8"));
      dataFile.close();
      Serial.print(F("Created new log file: "));
      Serial.println(currentLogFile);
    } else {
      Serial.println(F("Error opening log file for writing"));
    }
  }
}

void logCurrentReadings() {
  // Read temperature
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);
  if (temperature == DEVICE_DISCONNECTED_C) {
    temperature = -127.0;  // Invalid reading
  }
  
  // Read humidity
  float humidity = dht.readHumidity();
  if (isnan(humidity)) {
    humidity = -1.0;  // Invalid reading
  }
  
  // Read analog values
  int analog1 = analogRead(ANALOG_INPUT_1);
  int analog2 = analogRead(ANALOG_INPUT_2);
  
  // Read digital inputs
  byte inputState = inputModule.read8();
  
  // Create reading record
  SensorReading reading;
  reading.timestamp = timeClient.getEpochTime();
  reading.temperature = temperature;
  reading.humidity = humidity;
  reading.analog1 = analog1;
  reading.analog2 = analog2;
  reading.inputState = inputState;
  
  // Store in circular buffer
  readings[readingIndex] = reading;
  readingIndex = (readingIndex + 1) % MAX_READINGS_IN_MEMORY;
  if (readingCount < MAX_READINGS_IN_MEMORY) {
    readingCount++;
  }
  
  // Log to file
  logReadingToFile(reading);
  
  // Check for threshold violations
  if (alertsEnabled) {
    checkThresholds(reading);
  }
  
  // Debug output
  Serial.println(F("Reading logged:"));
  Serial.print(F("Time: "));
  Serial.println(getTimeStampString(reading.timestamp));
  Serial.print(F("Temperature: "));
  Serial.print(reading.temperature);
  Serial.println(F("°C"));
  Serial.print(F("Humidity: "));
  Serial.print(reading.humidity);
  Serial.println(F("%"));
  Serial.print(F("Analog1: "));
  Serial.println(reading.analog1);
  Serial.print(F("Analog2: "));
  Serial.println(reading.analog2);
  Serial.print(F("Input State: 0b"));
  Serial.println(reading.inputState, BIN);
  Serial.println();
}

void logReadingToFile(const SensorReading& reading) {
  // Check if we need a new log file (e.g., if the date has changed)
  time_t now = reading.timestamp;
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  
  char shouldBeFilename[32];
  sprintf(shouldBeFilename, "/data_%04d%02d%02d.csv", 
          timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
  
  if (currentLogFile != String(shouldBeFilename)) {
    currentLogFile = String(shouldBeFilename);
    createNewLogFile();
  }
  
  // Open file for appending
  File dataFile;
  if (sdCardAvailable) {
    dataFile = SD.open(currentLogFile, FILE_APPEND);
  } else {
    dataFile = SPIFFS.open(currentLogFile, "a");
  }
  
  if (dataFile) {
    // Format the timestamp
    String timestampStr = getTimeStampString(reading.timestamp);
    
    // Format the input states
    byte inputs = reading.inputState;
    
    // Write CSV line
    dataFile.print(timestampStr);
    dataFile.print(F(","));
    dataFile.print(reading.temperature);
    dataFile.print(F(","));
    dataFile.print(reading.humidity);
    dataFile.print(F(","));
    dataFile.print(reading.analog1);
    dataFile.print(F(","));
    dataFile.print(reading.analog2);
    
    // Add each input state
    for (int i = 0; i < 8; i++) {
      dataFile.print(F(","));
      dataFile.print((inputs >> i) & 1);
    }
    
    dataFile.println();
    dataFile.close();
  } else {
    Serial.println(F("Error opening log file for appending"));
  }
}

bool getLatestReadings(int count, String& result) {
  DynamicJsonDocument doc(16384);  // Adjust size based on the number of readings
  JsonArray readingsArray = doc.createNestedArray("readings");
  
  // Limit count to the actual number of readings
  if (count > readingCount) {
    count = readingCount;
  }
  
  // Calculate the starting index for the last 'count' readings
  int startIdx = (readingIndex - count + MAX_READINGS_IN_MEMORY) % MAX_READINGS_IN_MEMORY;
  
  // Add the readings to the JSON
  for (int i = 0; i < count; i++) {
    int idx = (startIdx + i) % MAX_READINGS_IN_MEMORY;
    SensorReading& reading = readings[idx];
    
    JsonObject readingObj = readingsArray.createNestedObject();
    readingObj["timestamp"] = getTimeStampString(reading.timestamp);
    readingObj["temperature"] = reading.temperature;
    readingObj["humidity"] = reading.humidity;
    readingObj["analog1"] = reading.analog1;
    readingObj["analog2"] = reading.analog2;
    
    JsonArray inputsArray = readingObj.createNestedArray("inputs");
    for (int j = 0; j < 8; j++) {
      inputsArray.add((reading.inputState >> j) & 1);
    }
  }
  
  // Add metadata
  doc["count"] = count;
  doc["total"] = readingCount;
  
  // Serialize to string
  serializeJson(doc, result);
  
  return true;
}

void checkThresholds(const SensorReading& reading) {
  // Check temperature thresholds
  if (reading.temperature != -127.0) {  // Valid reading
    if (reading.temperature > tempHighThreshold) {
      sendAlert("High temperature alert: " + String(reading.temperature) + "°C");
    } else if (reading.temperature < tempLowThreshold) {
      sendAlert("Low temperature alert: " + String(reading.temperature) + "°C");
    }
  }
  
  // Check humidity thresholds
  if (reading.humidity != -1.0) {  // Valid reading
    if (reading.humidity > humidityHighThreshold) {
      sendAlert("High humidity alert: " + String(reading.humidity) + "%");
    } else if (reading.humidity < humidityLowThreshold) {
      sendAlert("Low humidity alert: " + String(reading.humidity) + "%");
    }
  }
}

String getTimeStampString(time_t time) {
  struct tm timeinfo;
  gmtime_r(&time, &timeinfo);
  
  char buffer[25];
  sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", 
          timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
          timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  
  return String(buffer);
}

void handleRoot(AsyncWebServerRequest *request) {
  String html = F(
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
      "<title>KC-Link PRO A8 Data Logger</title>"
      "<meta name='viewport' content='width=device-width, initial-scale=1'>"
      "<link rel='stylesheet' type='text/css' href='style.css'>"
      "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>"
    "</head>"
    "<body>"
      "<div class='container'>"
        "<header>"
          "<h1>KC-Link PRO A8 Data Logger</h1>"
        "</header>"
        
        "<div class='status-panel'>"
          "<div id='status-message'>Loading system status...</div>"
          "<div class='status-details'>"
            "<div class='status-item'>"
              "<span class='label'>Temperature:</span>"
              "<span id='current-temp' class='value'>--</span>"
            "</div>"
            "<div class='status-item'>"
              "<span class='label'>Humidity:</span>"
              "<span id='current-humidity' class='value'>--</span>"
            "</div>"
            "<div class='status-item'>"
              "<span class='label'>Logging:</span>"
              "<span id='logging-status' class='value'>--</span>"
            "</div>"
          "</div>"
        "</div>"
        
        "<div class='chart-container'>"
          "<h2>Temperature History</h2>"
          "<canvas id='tempChart'></canvas>"
        "</div>"
        
        "<div class='chart-container'>"
          "<h2>Humidity History</h2>"
          "<canvas id='humidityChart'></canvas>"
        "</div>"
        
        "<div class='configuration-panel'>"
          "<h2>Configuration</h2>"
          "<form id='config-form'>"
            "<div class='form-group'>"
              "<label for='logInterval'>Log Interval (seconds):</label>"
              "<input type='number' id='logInterval' min='1' max='3600'>"
            "</div>"
            "<div class='form-group'>"
              "<label for='loggingEnabled'>Logging Enabled:</label>"
              "<input type='checkbox' id='loggingEnabled'>"
            "</div>"
            "<div class='form-group'>"
              "<label for='alertsEnabled'>Alerts Enabled:</label>"
              "<input type='checkbox' id='alertsEnabled'>"
            "</div>"
            "<div class='form-group'>"
              "<label for='tempHighThreshold'>Temperature High Threshold (°C):</label>"
              "<input type='number' id='tempHighThreshold' step='0.1'>"
            "</div>"
            "<div class='form-group'>"
              "<label for='tempLowThreshold'>Temperature Low Threshold (°C):</label>"
              "<input type='number' id='tempLowThreshold' step='0.1'>"
            "</div>"
            "<div class='form-group'>"
              "<label for='humidityHighThreshold'>Humidity High Threshold (%):</label>"
              "<input type='number' id='humidityHighThreshold' step='0.1'>"
            "</div>"
            "<div class='form-group'>"
              "<label for='humidityLowThreshold'>Humidity Low Threshold (%):</label>"
              "<input type='number' id='humidityLowThreshold' step='0.1'>"
            "</div>"
            "<button type='submit' class='btn'>Save Configuration</button>"
          "</form>"
        "</div>"
        
        "<div class='export-panel'>"
          "<h2>Data Export</h2>"
          "<p>Download data as CSV file:</p>"
          "<a href='/api/export' class='btn'>Export Data</a>"
        "</div>"
      "</div>"
      "<script src='script.js'></script>"
    "</body>"
    "</html>"
  );
  
  request->send(200, "text/html", html);
}

void handleStyleCss(AsyncWebServerRequest *request) {
  String css = F(
    "* {box-sizing: border-box; margin: 0; padding: 0;}"
    "body {font-family: Arial, sans-serif; line-height: 1.6; color: #333; background: #f4f4f4; padding: 20px;}"
    ".container {max-width: 1200px; margin: 0 auto; background: white; padding: 20px; border-radius: 5px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);}"
    "header {margin-bottom: 20px; padding-bottom: 10px; border-bottom: 1px solid #eee;}"
    "h1, h2 {color: #444;}"
    ".status-panel {background: #f9f9f9; padding: 15px; border-radius: 5px; margin-bottom: 20px;}"
    ".status-details {display: flex; flex-wrap: wrap; margin-top: 10px;}"
    ".status-item {margin-right: 20px; margin-bottom: 10px;}"
    ".label {font-weight: bold;}"
    ".value {margin-left: 5px;}"
    ".chart-container {margin-bottom: 30px;}"
    ".configuration-panel, .export-panel {background: #f9f9f9; padding: 15px; border-radius: 5px; margin-bottom: 20px;}"
    ".form-group {margin-bottom: 15px;}"
    "label {display: block; margin-bottom: 5px;}"
    "input[type='number'], input[type='text'] {width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px;}"
    "input[type='checkbox'] {transform: scale(1.5); margin-left: 5px;}"
    ".btn {display: inline-block; background: #4CAF50; color: white; padding: 10px 15px; border: none; border-radius: 4px; cursor: pointer; text-decoration: none;}"
    ".btn:hover {background: #45a049;}"
    "@media (max-width: 768px) {.status-details {flex-direction: column;} .status-item {margin-right: 0;}}"
  );
  
  request->send(200, "text/css", css);
}

void handleScriptJs(AsyncWebServerRequest *request) {
  String js = F(
    "document.addEventListener('DOMContentLoaded', function() {"
    "  // Fetch configuration"
    "  fetchConfig();"
    "  "
    "  // Fetch initial readings"
    "  fetchReadings();"
    "  "
    "  // Set up auto-refresh (every 30 seconds)"
    "  setInterval(fetchReadings, 30000);"
    "  "
    "  // Set up form submission"
    "  document.getElementById('config-form').addEventListener('submit', function(e) {"
    "    e.preventDefault();"
    "    saveConfig();"
    "  });"
    "});"
    ""
    "function fetchConfig() {"
    "  fetch('/api/config')"
    "    .then(response => response.json())"
    "    .then(data => {"
    "      document.getElementById('logInterval').value = data.logInterval;"
    "      document.getElementById('loggingEnabled').checked = data.loggingEnabled;"
    "      document.getElementById('alertsEnabled').checked = data.alertsEnabled;"
    "      document.getElementById('tempHighThreshold').value = data.tempHighThreshold;"
    "      document.getElementById('tempLowThreshold').value = data.tempLowThreshold;"
    "      document.getElementById('humidityHighThreshold').value = data.humidityHighThreshold;"
    "      document.getElementById('humidityLowThreshold').value = data.humidityLowThreshold;"
    "    })"
    "    .catch(error => console.error('Error fetching config:', error));"
    "}"
    ""
    "function saveConfig() {"
    "  const config = {"
    "    logInterval: parseInt(document.getElementById('logInterval').value),"
    "    loggingEnabled: document.getElementById('loggingEnabled').checked,"
    "    alertsEnabled: document.getElementById('alertsEnabled').checked,"
    "    tempHighThreshold: parseFloat(document.getElementById('tempHighThreshold').value),"
    "    tempLowThreshold: parseFloat(document.getElementById('tempLowThreshold').value),"
    "    humidityHighThreshold: parseFloat(document.getElementById('humidityHighThreshold').value),"
    "    humidityLowThreshold: parseFloat(document.getElementById('humidityLowThreshold').value)"
    "  };"
    "  "
    "  fetch('/api/config', {"
    "    method: 'POST',"
    "    headers: {'Content-Type': 'application/json'},"
    "    body: JSON.stringify(config)"
    "  })"
    "    .then(response => response.json())"
    "    .then(data => {"
    "      if (data.success) {"
    "        alert('Configuration saved successfully');"
    "      } else {"
    "        alert('Error saving configuration: ' + data.message);"
    "      }"
    "    })"
    "    .catch(error => console.error('Error saving config:', error));"
    "}"
    ""
    "let tempChart = null;"
    "let humidityChart = null;"
    ""
    "function fetchReadings() {"
    "  fetch('/api/readings?count=50')"
    "    .then(response => response.json())"
    "    .then(data => {"
    "      updateStatus(data);"
    "      updateCharts(data);"
    "    })"
    "    .catch(error => console.error('Error fetching readings:', error));"
    "}"
    ""
    "function updateStatus(data) {"
    "  if (data.readings && data.readings.length > 0) {"
    "    const latest = data.readings[data.readings.length - 1];"
    "    "
    "    document.getElementById('current-temp').textContent = latest.temperature.toFixed(1) + '°C';"
    "    document.getElementById('current-humidity').textContent = latest.humidity.toFixed(1) + '%';"
    "    document.getElementById('logging-status').textContent = 'Active';"
    "    document.getElementById('status-message').textContent = 'System running normally. Last update: ' + latest.timestamp;"
    "  } else {"
    "    document.getElementById('status-message').textContent = 'No readings available';"
    "  }"
    "}"
    ""
    "function updateCharts(data) {"
    "  if (!data.readings || data.readings.length === 0) return;"
    "  "
    "  const timestamps = data.readings.map(reading => reading.timestamp);"
    "  const temperatures = data.readings.map(reading => reading.temperature);"
    "  const humidities = data.readings.map(reading => reading.humidity);"
    "  "
    "  // Temperature chart"
    "  const tempCtx = document.getElementById('tempChart').getContext('2d');"
    "  if (tempChart) {"
    "    tempChart.data.labels = timestamps;"
    "    tempChart.data.datasets[0].data = temperatures;"
    "    tempChart.update();"
    "  } else {"
    "    tempChart = new Chart(tempCtx, {"
    "      type: 'line',"
    "      data: {"
    "        labels: timestamps,"
    "        datasets: [{"
    "          label: 'Temperature (°C)',"
    "          data: temperatures,"
    "          backgroundColor: 'rgba(255, 99, 132, 0.2)',"
    "          borderColor: 'rgba(255, 99, 132, 1)',"
    "          borderWidth: 1"
    "        }]"
    "      },"
    "      options: {"
    "        responsive: true,"
    "        scales: {"
    "          y: {"
    "            beginAtZero: false"
    "          }"
    "        }"
    "      }"
    "    });"
    "  }"
    "  "
    "  // Humidity chart"
    "  const humidityCtx = document.getElementById('humidityChart').getContext('2d');"
    "  if (humidityChart) {"
    "    humidityChart.data.labels = timestamps;"
    "    humidityChart.data.datasets[0].data = humidities;"
    "    humidityChart.update();"
    "  } else {"
    "    humidityChart = new Chart(humidityCtx, {"
    "      type: 'line',"
    "      data: {"
    "        labels: timestamps,"
    "        datasets: [{"
    "          label: 'Humidity (%)',"
    "          data: humidities,"
    "          backgroundColor: 'rgba(54, 162, 235, 0.2)',"
    "          borderColor: 'rgba(54, 162, 235, 1)',"
    "          borderWidth: 1"
    "        }]"
    "      },"
    "      options: {"
    "        responsive: true,"
    "        scales: {"
    "          y: {"
    "            beginAtZero: false"
    "          }"
    "        }"
    "      }"
    "    });"
    "  }"
    "}"
  );
  
  request->send(200, "application/javascript", js);
}

void handleGetReadings(AsyncWebServerRequest *request) {
  int count = 20;  // Default count
  
  // Check if count parameter is provided
  if (request->hasParam("count")) {
    count = request->getParam("count")->value().toInt();
  }
  
  // Limit count to a reasonable range
  if (count < 1) count = 1;
  if (count > 1000) count = 1000;
  
  String response;
  if (getLatestReadings(count, response)) {
    request->send(200, "application/json", response);
  } else {
    request->send(500, "application/json", "{\"error\":\"Failed to get readings\"}");
  }
}

void handleConfiguration(AsyncWebServerRequest *request) {
  DynamicJsonDocument doc(1024);
  
  doc["logInterval"] = logIntervalSeconds;
  doc["loggingEnabled"] = loggingEnabled;
  doc["alertsEnabled"] = alertsEnabled;
  doc["tempHighThreshold"] = tempHighThreshold;
  doc["tempLowThreshold"] = tempLowThreshold;
  doc["humidityHighThreshold"] = humidityHighThreshold;
  doc["humidityLowThreshold"] = humidityLowThreshold;
  doc["sdCardAvailable"] = sdCardAvailable;
  
  String response;
  serializeJson(doc, response);
  
  request->send(200, "application/json", response);
}

void handleSetConfig(AsyncWebServerRequest *request) {
  String body = request->arg("plain");
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
    return;
  }
  
  bool validConfig = true;
  String errorMessage = "";
  
  // Validate and apply settings
  if (doc.containsKey("logInterval")) {
    int interval = doc["logInterval"];
    if (interval >= 1 && interval <= 3600) {
      logIntervalSeconds = interval;
    } else {
      validConfig = false;
      errorMessage = "Log interval must be between 1 and 3600 seconds";
    }
  }
  
  if (doc.containsKey("loggingEnabled")) {
    loggingEnabled = doc["loggingEnabled"];
  }
  
  if (doc.containsKey("alertsEnabled")) {
    alertsEnabled = doc["alertsEnabled"];
  }
  
  if (doc.containsKey("tempHighThreshold")) {
    tempHighThreshold = doc["tempHighThreshold"];
  }
  
  if (doc.containsKey("tempLowThreshold")) {
    tempLowThreshold = doc["tempLowThreshold"];
  }
  
  if (doc.containsKey("humidityHighThreshold")) {
    humidityHighThreshold = doc["humidityHighThreshold"];
  }
  
  if (doc.containsKey("humidityLowThreshold")) {
    humidityLowThreshold = doc["humidityLowThreshold"];
  }
  
  // Save configuration if valid
  if (validConfig) {
    saveConfiguration();
    request->send(200, "application/json", "{\"success\":true}");
  } else {
    request->send(400, "application/json", "{\"success\":false,\"message\":\"" + errorMessage + "\"}");
  }
}

void handleExportData(AsyncWebServerRequest *request) {
  // Check if any data is available
  if (readingCount == 0) {
    request->send(404, "text/plain", "No data available for export");
    return;
  }
  
  // Generate a filename based on current date
  time_t now = timeClient.getEpochTime();
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  
  char filename[32];
  sprintf(filename, "data_export_%04d%02d%02d.csv", 
          timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
  
  // Set headers for file download
  AsyncResponseStream *response = request->beginResponseStream("text/csv");
  response->addHeader("Content-Disposition", "attachment; filename=" + String(filename));
  
  // Add CSV header
  response->print(F("Timestamp,Temperature,Humidity,Analog1,Analog2,Input1,Input2,Input3,Input4,Input5,Input6,Input7,Input8\r\n"));
  
  // Add data from memory buffer
  int startIdx = (readingIndex - readingCount + MAX_READINGS_IN_MEMORY) % MAX_READINGS_IN_MEMORY;
  for (int i = 0; i < readingCount; i++) {
    int idx = (startIdx + i) % MAX_READINGS_IN_MEMORY;
    SensorReading& reading = readings[idx];
    
    // Format CSV line
    response->print(getTimeStampString(reading.timestamp));
    response->print(F(","));
    response->print(reading.temperature);
    response->print(F(","));
    response->print(reading.humidity);
    response->print(F(","));
    response->print(reading.analog1);
    response->print(F(","));
    response->print(reading.analog2);
    
    // Add each input state
    for (int j = 0; j < 8; j++) {
      response->print(F(","));
      response->print((reading.inputState >> j) & 1);
    }
    
    response->print(F("\r\n"));
  }
  
  request->send(response);
}

void sendAlert(const String& message) {
  Serial.print(F("ALERT: "));
  Serial.println(message);
  
  // Here you could implement various alert mechanisms:
  // 1. Activate a relay to sound an alarm
  // 2. Send an email notification
  // 3. Send an MQTT message to a broker
  // 4. Send an HTTP request to a webhook service
  
  // For demonstration, we'll just toggle the first relay
  bool currentState = (state.relayState >> 0) & 1;
  relayModule.digitalWrite(0, !currentState);  // Toggle relay
  
  // Turn it back after 1 second
  delay(1000);
  relayModule.digitalWrite(0, currentState);  // Restore original state
}

// Main entry point for periodic execution
// The loop() function calls this at the specified interval
void logCurrentReadings() {
  // Read temperature
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);
  if (temperature == DEVICE_DISCONNECTED_C) {
    temperature = -127.0;  // Invalid reading
  }
  
  // Read humidity
  float humidity = dht.readHumidity();
  if (isnan(humidity)) {
    humidity = -1.0;  // Invalid reading
  }
  
  // Read analog values
  int analog1 = analogRead(ANALOG_INPUT_1);
  int analog2 = analogRead(ANALOG_INPUT_2);
  
  // Read digital inputs
  byte inputState = inputModule.read8();
  
  // Create reading record
  SensorReading reading;
  reading.timestamp = timeClient.getEpochTime();
  reading.temperature = temperature;
  reading.humidity = humidity;
  reading.analog1 = analog1;
  reading.analog2 = analog2;
  reading.inputState = inputState;
  
  // Store in circular buffer
  readings[readingIndex] = reading;
  readingIndex = (readingIndex + 1) % MAX_READINGS_IN_MEMORY;
  if (readingCount < MAX_READINGS_IN_MEMORY) {
    readingCount++;
  }
  
  // Log to file
  logReadingToFile(reading);
  
  // Check for threshold violations
  if (alertsEnabled) {
    checkThresholds(reading);
  }
  
  // Debug output
  Serial.println(F("Reading logged:"));
  Serial.print(F("Time: "));
  Serial.println(getTimeStampString(reading.timestamp));
  Serial.print(F("Temperature: "));
  Serial.print(reading.temperature);
  Serial.println(F("°C"));
  Serial.print(F("Humidity: "));
  Serial.print(reading.humidity);
  Serial.println(F("%"));
  Serial.print(F("Analog1: "));
  Serial.println(reading.analog1);
  Serial.print(F("Analog2: "));
  Serial.println(reading.analog2);
  Serial.print(F("Input State: 0b"));
  Serial.println(reading.inputState, BIN);
  Serial.println();
}

// Optional: MQTT Integration
// This code could be added to support MQTT publishing of sensor data
/*
#include <PubSubClient.h>

// MQTT settings
const char* mqtt_server = "mqtt.example.com";
const int mqtt_port = 1883;
const char* mqtt_username = "user";
const char* mqtt_password = "password";
const char* clientID = "KC-Link-DataLogger";

// MQTT topics
const char* tempTopic = "kc-link/temperature";
const char* humidityTopic = "kc-link/humidity";
const char* analogTopic = "kc-link/analog";
const char* inputsTopic = "kc-link/inputs";

// MQTT client
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Initialize MQTT
void setupMQTT() {
  mqttClient.setServer(mqtt_server, mqtt_port);
  reconnectMQTT();
}

// Connect/reconnect to MQTT broker
void reconnectMQTT() {
  if (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT broker...");
    if (mqttClient.connect(clientID, mqtt_username, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.println(mqttClient.state());
    }
  }
}

// Publish sensor data to MQTT
void publishToMQTT(const SensorReading& reading) {
  // Ensure connected
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  
  // Publish temperature
  if (reading.temperature != -127.0) {
    mqttClient.publish(tempTopic, String(reading.temperature).c_str());
  }
  
  // Publish humidity
  if (reading.humidity != -1.0) {
    mqttClient.publish(humidityTopic, String(reading.humidity).c_str());
  }
  
  // Publish analog values
  String analogJson = "{\"analog1\":" + String(reading.analog1) + 
                      ",\"analog2\":" + String(reading.analog2) + "}";
  mqttClient.publish(analogTopic, analogJson.c_str());
  
  // Publish digital inputs
  String inputsJson = "{\"inputs\":" + String(reading.inputState) + "}";
  mqttClient.publish(inputsTopic, inputsJson.c_str());
}
*/

// Optional: Email Alert System
// This code could be added to send email alerts
/*
#include <ESP_Mail_Client.h>

// Email settings
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
#define AUTHOR_EMAIL "your-email@gmail.com"
#define AUTHOR_PASSWORD "your-app-password"
#define RECIPIENT_EMAIL "recipient@example.com"

// Global email objects
SMTPSession smtp;
SMTP_Message message;

// Initialize email client
void setupEmail() {
  smtp.debug(1); // Set debug level (0 for no debug)
  
  // Set the callback function to get the sending results
  smtp.callback(smtpCallback);
}

// Callback function to get the Email sending status
void smtpCallback(SMTP_Status status) {
  if (status.success()) {
    Serial.println("Email sent successfully");
  } else {
    Serial.println("Error sending email");
  }
}

// Send email alert
void sendEmailAlert(const String& subject, const String& body) {
  // Connect to SMTP server with the session config
  ESP_Mail_Session session;
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "";
  
  // Set up message parameters
  message.sender.name = "KC-Link DataLogger";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = subject;
  message.addRecipient("Recipient", RECIPIENT_EMAIL);
  message.text.content = body.c_str();
  
  // Connect to server and send email
  if (!smtp.connect(&session)) {
    Serial.println("Error connecting to SMTP server");
    return;
  }
  
  if (!MailClient.sendMail(&smtp, &message)) {
    Serial.println("Error sending email: " + smtp.errorReason());
  }
}
*/

// Optional: SD Card File Browser API
// This code could be added to browse and download files from the SD card
/*
// Add these API endpoints to setupWebServer()

// List files on SD card
server.on("/api/files", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (!sdCardAvailable) {
    request->send(404, "application/json", "{\"error\":\"SD card not available\"}");
    return;
  }
  
  DynamicJsonDocument doc(4096);
  JsonArray files = doc.createNestedArray("files");
  
  File root = SD.open("/");
  if (!root) {
    request->send(500, "application/json", "{\"error\":\"Could not open root directory\"}");
    return;
  }
  
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      JsonObject fileObj = files.createNestedObject();
      fileObj["name"] = String(file.name());
      fileObj["size"] = file.size();
      
      // Get last modified time (if available)
      time_t lastWrite = file.getLastWrite();
      if (lastWrite) {
        fileObj["modified"] = getTimeStampString(lastWrite);
      }
    }
    file = root.openNextFile();
  }
  
  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
});

// Download specific file from SD card
server.on("/api/files/download", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (!sdCardAvailable) {
    request->send(404, "text/plain", "SD card not available");
    return;
  }
  
  if (!request->hasParam("filename")) {
    request->send(400, "text/plain", "Missing filename parameter");
    return;
  }
  
  String filename = request->getParam("filename")->value();
  
  if (!SD.exists(filename)) {
    request->send(404, "text/plain", "File not found");
    return;
  }
  
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    request->send(500, "text/plain", "Failed to open file");
    return;
  }
  
  AsyncResponseStream *response = request->beginResponseStream("application/octet-stream");
  response->addHeader("Content-Disposition", "attachment; filename=" + filename);
  
  uint8_t buffer[1024];
  while (file.available()) {
    size_t len = file.read(buffer, sizeof(buffer));
    response->write(buffer, len);
  }
  
  file.close();
  request->send(response);
});
*/

// Optional: Advanced Data Visualization
// This JavaScript code could be added to the web interface
/*
// Add this to the script.js response in handleScriptJs()

// Function to create and display a heat map of sensor data
function createHeatMap(data) {
  // Get data for heat map
  const timestamps = data.readings.map(reading => reading.timestamp);
  const temperatures = data.readings.map(reading => reading.temperature);
  
  // Create a data grid for the heat map
  const hoursOfDay = [...Array(24).keys()];
  const daysOfWeek = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];
  
  // Create empty 2D array (7x24) filled with null
  const heatmapData = Array(7).fill().map(() => Array(24).fill(null));
  
  // Populate the grid with temperature data
  data.readings.forEach((reading, index) => {
    const date = new Date(reading.timestamp);
    const dayOfWeek = date.getDay();
    const hourOfDay = date.getHours();
    
    // Update the value (or average multiple values for the same hour)
    if (heatmapData[dayOfWeek][hourOfDay] === null) {
      heatmapData[dayOfWeek][hourOfDay] = reading.temperature;
    } else {
      // Average with existing value
      heatmapData[dayOfWeek][hourOfDay] = 
        (heatmapData[dayOfWeek][hourOfDay] + reading.temperature) / 2;
    }
  });
  
  // Find min/max for color scaling
  let min = 100, max = -100;
  heatmapData.forEach(row => {
    row.forEach(value => {
      if (value !== null) {
        min = Math.min(min, value);
        max = Math.max(max, value);
      }
    });
  });
  
  // Create chart context
  const ctx = document.getElementById('heatmapChart').getContext('2d');
  
  // Create chart data
  const chartData = {
    labels: hoursOfDay,
    datasets: daysOfWeek.map((day, index) => ({
      label: day,
      data: heatmapData[index],
      backgroundColor: function(context) {
        const value = context.dataset.data[context.dataIndex];
        if (value === null) return 'rgba(0, 0, 0, 0.1)';
        
        // Calculate color based on temperature (blue to red)
        const percent = (value - min) / (max - min);
        const r = Math.floor(255 * percent);
        const b = Math.floor(255 * (1 - percent));
        return `rgba(${r}, 0, ${b}, 0.8)`;
      }
    }))
  };
  
  // Create heatmap chart
  new Chart(ctx, {
    type: 'matrix',
    data: chartData,
    options: {
      responsive: true,
      maintainAspectRatio: false,
      title: {
        display: true,
        text: 'Temperature Heatmap by Day and Hour'
      },
      tooltip: {
        callbacks: {
          title: function(tooltipItem, data) {
            return `${daysOfWeek[tooltipItem[0].datasetIndex]}, ${tooltipItem[0].index}:00`;
          },
          label: function(tooltipItem, data) {
            const value = data.datasets[tooltipItem.datasetIndex].data[tooltipItem.index];
            return value !== null ? `${value.toFixed(1)}°C` : 'No data';
          }
        }
      }
    }
  });
}
*/
/**
 * KC-Link PRO A8 Data Logger
 * 
 * This sketch turns the KC-Link PRO A8 into a comprehensive data logger
 * that records sensor readings, input states, and system events to an
 * SD card or internal memory. It includes a web interface for viewing
 * historical data and configuring logging parameters.
 * 
 * Features:
 * - Records temperature, humidity, analog inputs, and digital inputs
 * - Configurable logging intervals
 * - Data export via web interface or API
 * - Visual graphs of historical data
 * - Optional SD card support for extended storage
 * - Email alerts for threshold violations
 * 
 * Hardware:
 * - KC-Link PRO A8 board
 * - DS18B20 temperature sensors
 * - DHT22 humidity sensor
 * - Optional: micro SD card module connected to SPI pins
 */

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <PCF8574.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <FS.h>
#include <TimeLib.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <SPI.h>
#include <SD.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// PCF8574 addresses
#define PCF8574_RELAY_ADDR 0x20  // Address for relay control
#define PCF8574_INPUT_ADDR 0x22  // Address for digital inputs

// Pin definitions for KC-Link PRO A8 V1.4
#define ANALOG_INPUT_1 34
#define ANALOG_INPUT_2 35
#define TEMP_SENSOR_1_PIN 14
#define DHTPIN 13
#define DHTTYPE DHT22

// SD card pin if using SPI SD card module
#define SD_CS_PIN 5

// Configuration
const int DEFAULT_LOG_INTERVAL = 60;  // Default logging interval in seconds
const int MAX_READINGS_IN_MEMORY = 1000;  // Maximum readings to keep in memory
const String LOG_FILE_PREFIX = "/data_log_";
const String CONFIG_FILE = "/logger_config.json";

// Global objects
PCF8574 relayModule(PCF8574_RELAY_ADDR);
PCF8574 inputModule(PCF8574_INPUT_ADDR);
OneWire oneWire(TEMP_SENSOR_1_PIN);
DallasTemperature sensors(&oneWire);
DHT dht(DHTPIN, DHTTYPE);
AsyncWebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);
bool sdCardAvailable = false;

// Global variables
int logIntervalSeconds = DEFAULT_LOG_INTERVAL;
bool loggingEnabled = true;
bool alertsEnabled = true;
float tempHighThreshold = 30.0;
float tempLowThreshold = 10.0;
float humidityHighThreshold = 70.0;
float humidityLowThreshold = 30.0;
String currentLogFile;
unsigned long lastLogTime = 0;

// Structure for storing sensor readings
struct SensorReading {
  time_t timestamp;
  float temperature;
  float humidity;
  int analog1;
  int analog2;
  byte inputState;
};

// Circular buffer for storing recent readings in memory
SensorReading readings[MAX_READINGS_IN_MEMORY];
int readingCount = 0;
int readingIndex = 0;

// Function declarations
void setupWiFi();
void setupFileSystem();
void setupSensors();
void setupWebServer();
void loadConfiguration();
void saveConfiguration();
void createNewLogFile();
void logCurrentReadings();
void logReadingToFile(const SensorReading& reading);
bool getLatestReadings(int count, String& result);
void checkThresholds(const SensorReading& reading);
String getTimeStampString(time_t time);
void handleRoot(AsyncWebServerRequest *request);
void handleStyleCss(AsyncWebServerRequest *request);
void handleScriptJs(AsyncWebServerRequest *request);
void handleGetReadings(AsyncWebServerRequest *request);
void handleConfiguration(AsyncWebServerRequest *request);
void handleSetConfig(AsyncWebServerRequest *request);
void handleExportData(AsyncWebServerRequest *request);
void sendAlert(const String& message);

void setup() {
  // Initialize serial port
  Serial.begin(115200);
  Serial.println(F("KC-Link PRO A8 Data Logger"));
  
  // Initialize I2C
  Wire.begin();
  
  // Setup components
  setupFileSystem();
  setupSensors();
  setupWiFi();
  
  // Load configuration
  loadConfiguration();
  
  // Get time from NTP
  timeClient.begin();
  timeClient.update();
  setTime(timeClient.getEpochTime());
  
  // Create a new log file
  createNewLogFile();
  
  // Setup web server
  setupWebServer();
  
  Serial.println(F("Data logger initialized and ready"));
}

void loop() {
  // Update time
  timeClient.update();
  setTime(timeClient.getEpochTime());
  
  // Check if it's time to log data
  if (loggingEnabled && (millis() - lastLogTime > (logIntervalSeconds * 1000))) {
    lastLogTime = millis();
    logCurrentReadings();
  }
  
  // Small delay to avoid hogging the CPU
  delay(100);
}

void setupWiFi() {
  Serial.println(F("Connecting to WiFi..."));
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println(F("WiFi connected"));
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println(F("WiFi connection failed. Operating in offline mode."));
  }
}

void setupFileSystem() {
  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println(F("An error occurred while mounting SPIFFS"));
  } else {
    Serial.println(F("SPIFFS mounted successfully"));
  }
  
  // Check for SD card
  if (
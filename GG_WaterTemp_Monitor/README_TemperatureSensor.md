# Temperature Sensor Manager

This project implements a comprehensive temperature sensor management system for ESP32 using DS18B20 sensors.

## File Structure

### `src/TemperatureSensor.h`
Header file containing:
- `SensorInfo` structure definition
- `TemperatureSensorManager` class declaration
- Function prototypes and constants

### `src/TemperatureSensor.cpp` 
Implementation file containing:
- Complete TemperatureSensorManager class implementation
- All sensor management functions
- Helper functions for address conversion and comparison

### `src/main.cpp`
Main application file containing:
- Setup and loop functions
- Example usage of the TemperatureSensorManager class

## Key Features

- **Persistent Sensor Identification**: Sensors are tracked by their unique ROM addresses
- **Custom Naming**: Assign meaningful names to sensors (e.g., "Pool_Water", "Ambient_Air")
- **Automatic Registration**: Sensors are automatically detected and registered with sequential IDs
- **Connection Status Tracking**: Monitor if sensors are active or disconnected
- **Individual Temperature Reading**: Get readings from specific sensors by ID

## Usage Examples

### Basic Usage
```cpp
// Create manager instance
TemperatureSensorManager tempSensorManager(ONE_WIRE_BUS);

// Initialize and scan
tempSensorManager.begin();
tempSensorManager.scanSensors();

// Read all temperatures
tempSensorManager.printTemperatureReadings();
```

### Advanced Usage
```cpp
// Rename sensors
tempSensorManager.renameSensor(0, "Pool_Temperature");
tempSensorManager.renameSensor(1, "Air_Temperature");

// Get individual readings
float poolTemp = tempSensorManager.getTemperatureC(0);
float airTemp = tempSensorManager.getTemperatureF(1);

// Check sensor status
if (tempSensorManager.isSensorActive(0)) {
    Serial.println("Pool sensor is connected");
}

// Get sensor information
String poolSensorName = tempSensorManager.getSensorName(0);
String poolSensorAddress = tempSensorManager.getSensorAddress(0);
```

## Class Methods

### Initialization
- `TemperatureSensorManager(int busPin)` - Constructor
- `begin()` - Initialize the sensor system
- `scanSensors()` - Scan for and register sensors

### Reading Data
- `printTemperatureReadings()` - Print all sensor readings
- `getTemperatureC(int sensorId)` - Get temperature in Celsius
- `getTemperatureF(int sensorId)` - Get temperature in Fahrenheit

### Sensor Management
- `renameSensor(int sensorId, String newName)` - Rename a sensor
- `printSensorInfo()` - List all registered sensors
- `getSensorCount()` - Get total number of registered sensors
- `isSensorActive(int sensorId)` - Check if sensor is connected
- `getSensorName(int sensorId)` - Get sensor name
- `getSensorAddress(int sensorId)` - Get sensor ROM address

## Configuration

- `MAX_SENSORS`: Maximum number of sensors supported (default: 10)
- `ONE_WIRE_BUS`: GPIO pin for OneWire communication (default: 0)

## Wiring

- VCC: 3.3V or 5V
- GND: Ground
- Data: GPIO 0 (or your chosen pin) with 4.7kΩ pull-up resistor to VCC
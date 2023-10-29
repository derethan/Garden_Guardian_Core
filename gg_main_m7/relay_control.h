void setRelay1 (int RELAY_PIN, float temperature1, float targetTemperature) {
      // Relay 1
    if (temperature1 <= targetTemperature) {
        digitalWrite(RELAY_PIN, LOW);
    } else {
        digitalWrite(RELAY_PIN, HIGH);
    }
}
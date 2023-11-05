//Initialize LCD
void useLCD () {
  lcd.init(); // initialize the LCD
  lcd.backlight();
  lcd.createChar(0, Heart);
  lcd.createChar(1, Cup);
  lcd.createChar(2, Shield);
  lcd.createChar(3, LineTop);
  lcd.createChar(4, arrowleft);
  lcd.createChar(5, arrowright);
}

//Display the Boot Screen
void bootScreen () {

  lcd.clear();
  lcd.setCursor(8, 1);
  lcd.print("BAMF");
  lcd.setCursor(2, 2);
  lcd.print("Garden Guardian");
  lcd.setCursor(17, 2);
  lcd.write(byte(0));
  delay(1000);
  lcd.setCursor(17, 2);
  lcd.write(byte(2));
  delay(1000);
  lcd.setCursor(17, 2);
  lcd.write(byte(0));
  delay(1000);
  lcd.setCursor(17, 2);
  lcd.write(byte(2));
  delay(1000);
  lcd.setCursor(17, 2);
  lcd.write(byte(0));
  delay(1000);
  lcd.setCursor(17, 2);
  lcd.write(byte(2));
  delay(1000);
  lcd.setCursor(17, 2);
  lcd.write(byte(0));
  delay(1000);
  lcd.clear();

}

// Function to display DHT sensor data
void displayDHTData(float temperature, float humidity, int currentPage, int lastPage) {
    // Clear the LCD

    if (currentPage != lastPage){
      lcd.clear();
    }
    

    // Display the data on the LCD
    lcd.setCursor(0, 0);
    lcd.write(byte(4));
    lcd.setCursor(1, 0);
    lcd.print("Grow Area Temp. #1");
    lcd.setCursor(19, 0);
    lcd.write(byte(5));
    lcd.setCursor(0, 1);
    for (int i = 0; i < 20; i++) {
        lcd.write(byte(3));  // Display the custom character Line 20 times
    }
    lcd.setCursor(0, 2);
    lcd.print("Temperature: " + String(temperature) + " C");
    lcd.setCursor(0, 3);
    lcd.print("Humidity: " + String(humidity) + " %");
}

// Function to display ambient temperature
void displayAmbientTemp(float ambientTemp, int currentPage, int lastPage) {

    if (currentPage != lastPage){
      lcd.clear();
    }

    // Display ambient temperature data on the LCD
    lcd.setCursor(0, 0);
    lcd.write(byte(4));
    lcd.setCursor(2, 0);
    lcd.print("Room Temperature");
    lcd.setCursor(19, 0);
    lcd.write(byte(5));
    lcd.setCursor(0, 1);
    for (int i = 0; i < 20; i++) {
        lcd.write(byte(3));  // Display the custom character Line 20 times
    }
    lcd.setCursor(0, 2);
    lcd.print("Temperature: " + String(ambientTemp) + " C");
}



// Function to display Heater Relay Screen
void displayHeaterStatus(int RELAY_PIN, float temperature, float targetTemperature, int currentPage, int lastPage) {
    // Read relay status
    int relayStatus = digitalRead(RELAY_PIN);
    
    if (currentPage != lastPage){
      lcd.clear();
    }

    // Display the Title on the LCD
    lcd.setCursor(0, 0);
    lcd.write(byte(4));
    lcd.setCursor(2, 0);

    if (relayStatus == LOW) {
        lcd.print("Heater is ON");
    } else {
        lcd.print("Heater is OFF");
    }
    lcd.setCursor(19, 0);
    lcd.write(byte(5));
    lcd.setCursor(0, 1);
    for (int i = 0; i < 20; i++) {
        lcd.write(byte(3));  // Display the custom character Line 20 times
    }

        lcd.setCursor(0, 2);
        lcd.print("Temp: " + String(temperature) + " C");

        lcd.setCursor(0, 3);
        lcd.print("Target: " + String(targetTemperature) + " C");
}

//Function to display the change Target Temperature Screen
void displayTempChange (float targetTemperature, int currentPage, int lastPage){

    if (currentPage != lastPage){
      lcd.clear();
    }

    //Display the Tearget Temperature on the LCD
    lcd.setCursor(0, 0);
    lcd.write(byte(4));
    lcd.setCursor(2, 0);
    lcd.print("Set Temperature");
    lcd.setCursor(19, 0);
    lcd.write(byte(5));
    lcd.setCursor(0, 1);
    for (int i = 0; i < 20; i++) {
        lcd.write(byte(3));  // Display the custom character Line 20 times
    }

    lcd.setCursor(0, 2);
    lcd.print("Temperature: " + String(targetTemperature) + " C");
}


// Function to display water flow data
void displayWaterFlow(int currentPage, int lastPage) {

    if (currentPage != lastPage){
      lcd.clear();
    }
    
    lcd.setCursor(0, 0);
    lcd.write(byte(4));
    lcd.setCursor(1, 0);
    lcd.print("Water Flow Monitor");
    lcd.setCursor(19, 0);
    lcd.write(byte(5));
    lcd.setCursor(0, 1);
    for (int i = 0; i < 20; i++) {
        lcd.write(byte(3));  // Display the custom character Line 20 times
    }
}
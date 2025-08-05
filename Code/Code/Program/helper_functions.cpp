#include "helper_functions.h"

bool signupOk = false;
bool Decision1 = false;
bool Decision2 = false;
bool MasterControl = false; // true means that other control signals are significant.
bool ManualControl = false;
uint16_t Water = 0;
uint16_t Moisture = 0;
float Temperature = 0;
float Humidity = 0;
String PumpStatus = "";

FirebaseData cloud;
FirebaseConfig config;
FirebaseAuth auth;
DHTesp dht;
SSD1306Wire display(0x3c, D2, D1);

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  onWiFiConnect();
}

void onWiFiConnect() {
  Serial.println("");
  Serial.print("Connected: ");
  Serial.print(ssid);
  Serial.println("");
  Serial.print(WiFi.localIP());
  Serial.println("");  
}

void initFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  Firebase.signUp(&config,&auth,"","") ? onFirebaseSignUp() : onFirebaseSignUpError();
  Firebase.begin(&config,&auth);
  Firebase.reconnectWiFi(true);
}

void onFirebaseSignUp() {
  signupOk = true;
  Serial.println("OK");
}

void onFirebaseSignUpError() {
  Serial.printf("%s\n", config.signer.signupError.message.c_str());
  Serial.println("Error");
}

void initDHTSensor() {
  dht.setup(DHTpin, DHTesp::DHT11);
}

void initAnalogSensors() {
  pinMode(MuxSelectPin, OUTPUT);  
}

void initPump() {
  digitalWrite(PunpPin, LOW);
  pinMode(PunpPin, OUTPUT);
}

void initDisplay() {
  display.init();
  display.clear();  
}

bool getDHTData() {
  delay(dht.getMinimumSamplingPeriod());
  Humidity = dht.getHumidity();
  Temperature = dht.getTemperature();
  Serial.printf("DHT11 Status String: %s\nHumidity = %.1f%%\nTemperature = %.1f\u2103\n", dht.getStatusString(), Humidity, Temperature);
  return (Temperature >= TempHigh);
}

bool getMoistureData() {
  digitalWrite(MuxSelectPin, 0);  // Moisture level will be measured
  Moisture = analogRead(AnalogPin);
  Serial.printf("Moisture Level: %.2f%%\n%s\n",
    Moisture * 100.0 / 1024.0, (Moisture < Moisture_Wet) ? "Status: Soil is too wet" :
      (Moisture < Moisture_Dry) ? "Status: Soil moisture is perfect" :
      "Status: Soil is too dry - time to water!");
  return (Moisture >= Moisture_Dry);
}

bool getWaterData() {
  digitalWrite(MuxSelectPin, 1); // Water level will be measured
  Water = analogRead(AnalogPin);
  Serial.printf("Water Level: %.2f%%\nStatus: Water level is %s\n",
    Water * 100.0 / 1024.0, (Water < Water_Low) ? "LOW" : (Water < Water_High) ? "MEDIUM" : "HIGH");
  return (Water < Water_High);
}

void controlPunp() {
  if (MasterControl) { // if water level is not high
    if(Firebase.ready() && signupOk) {
      Firebase.RTDB.setString(&cloud, MASTERCONTROL_URL, "1");
      PumpStatus = Firebase.RTDB.getString(&cloud, PUMP_URL) ? cloud.stringData() : "";
    }
    ManualControl = (PumpStatus == "1");
    digitalWrite(PunpPin, (PumpStatus == "1" || Decision1 || Decision2) ? HIGH : LOW); // Manual Control
  } else { // if water level is high, Close the pump if it is open
      Firebase.RTDB.setString(&cloud, MASTERCONTROL_URL, "0");
      Firebase.RTDB.setString(&cloud, PUMP_URL, "0");
      digitalWrite(PunpPin, 0);
      ManualControl = false;
      PumpStatus = "0";      
  }
}

void updateFirebase() {
  Firebase.RTDB.setFloat(&cloud, TEMPERATURE_URL, Temperature);
  Firebase.RTDB.setFloat(&cloud, HUMIDITY_URL, Humidity);
  Firebase.RTDB.setInt(&cloud, WATER_URL, Water * 100 / 1024);
  Firebase.RTDB.setInt(&cloud, MOISTURE_URL, Moisture * 100 / 1024);
}

void updateScreen() {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Temperature = " + String(Temperature, 1) + String("\u2103"));
  display.drawString(0, 12, "Humidity = " + String(Humidity, 1) + "%");
  display.drawString(0, 24, "Moisture = " + String(Moisture * 100 / 1024) + "%");
  display.drawString(0, 36, "Water = " + String(Water * 100 / 1024) + "%");
  ManualControl ? display.drawString(0, 48, "Manual Control: ON") : display.drawString(0, 48, "Manual Control: OFF");
  display.display();
}
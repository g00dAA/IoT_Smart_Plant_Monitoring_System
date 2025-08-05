#define DHTPIN 5
#define MaxCount 1e6
#define OVERLIMIT -1

uint8_t data[5];
float humidity, temperature;
uint32_t cycles[80];
uint32_t count = 0;

void setup() {
  Serial.begin(9600);
}
void loop() {
  delay(2000);
  data[0] = data[1] = data[2] = data[3] = data[4] = 0;
  // Start
  pinMode(DHTPIN, OUTPUT);
  digitalWrite(DHTPIN, LOW);
  delay(20);
  // Request
  pinMode(DHTPIN, INPUT_PULLUP);
  delayMicroseconds(50);
  count = 0;
  while (digitalRead(DHTPIN) == LOW && count++ < MaxCount) {}
  if (count >= MaxCount) return TIMEOUT;
  count = 0;
  while (digitalRead(DHTPIN) == HIGH && count++ < MaxCount) {}
  if (count >= MaxCount) return TIMEOUT;

  for (int i = 0; i < 80; i += 2) {
    count = 0;
    while (digitalRead(DHTPIN) == LOW && ++count) {}
    cycles[i] = count;
    count = 0;
    while (digitalRead(DHTPIN) == HIGH && ++count) {}
    cycles[i + 1] = count;
  }
  for (int i = 0; i < 40; ++i) {
    uint32_t lowCycles = cycles[2 * i];
    uint32_t highCycles = cycles[2 * i + 1];
    data[i / 8] <<= 1;
    if (highCycles > lowCycles) data[i / 8] |= 1;
  }
  if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
    humidity = data[0] + data[1] * 0.1;
    temperature = data[2] + (data[3] & 0x0f) * 0.1;
  }
  Serial.println("Humidity: " + String(humidity) + "%\tTemperature: " + String(temperature) + " degC");
}

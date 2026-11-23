/*
  DHT11 humidity controls the Arduino Uno onboard LED.

  Wiring (matches the supplied Uno + DHT11 diagram):
    DHT11 S   -> Uno D2
    DHT11 +   -> Uno 5V
    DHT11 g   -> Uno GND

  The onboard LED is on Uno D13 and is active HIGH.
  Higher humidity produces faster blinking.

  Required libraries:
    - DHT sensor library
    - Adafruit Unified Sensor
*/

#include "DHT.h"

constexpr uint8_t DHT_PIN = 2;
constexpr uint8_t DHT_TYPE = DHT11;
constexpr uint8_t LED_PIN = LED_BUILTIN;

constexpr float HUMIDITY_MIN = 20.0;
constexpr float HUMIDITY_MAX = 80.0;
constexpr unsigned long SLOW_BLINK_MS = 1600;
constexpr unsigned long FAST_BLINK_MS = 200;
constexpr unsigned long READ_INTERVAL_MS = 2000;

DHT dht(DHT_PIN, DHT_TYPE);

unsigned long lastReadMs = 0;
unsigned long lastBlinkMs = 0;
unsigned long blinkIntervalMs = SLOW_BLINK_MS;
bool ledState = false;
float lastValidHumidity = NAN;

void updateBlinkInterval(float humidity) {
  const float normalized =
      constrain((humidity - HUMIDITY_MIN) / (HUMIDITY_MAX - HUMIDITY_MIN),
                0.0, 1.0);
  blinkIntervalMs = static_cast<unsigned long>(
      SLOW_BLINK_MS + normalized * (FAST_BLINK_MS - SLOW_BLINK_MS));
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  dht.begin();
  Serial.println(F("DHT11 humidity test"));
  Serial.println(F("Humidity -> onboard LED blink rate"));
}

void loop() {
  const unsigned long now = millis();

  if (now - lastReadMs >= READ_INTERVAL_MS) {
    lastReadMs = now;

    const float humidity = dht.readHumidity();
    const float temperatureC = dht.readTemperature();

    if (isnan(humidity) || isnan(temperatureC)) {
      Serial.println(F("Failed to read from DHT sensor; retrying."));
    } else if (humidity < 0.0 || humidity > 100.0) {
      Serial.println(F("Invalid humidity reading; retrying."));
    } else {
      lastValidHumidity = humidity;
      updateBlinkInterval(humidity);

      Serial.print(F("Humidity: "));
      Serial.print(humidity, 1);
      Serial.print(F("%  Temperature: "));
      Serial.print(temperatureC, 1);
      Serial.print(F(" C  Blink interval: "));
      Serial.print(blinkIntervalMs);
      Serial.println(F(" ms"));
    }
  }

  if (!isnan(lastValidHumidity) && now - lastBlinkMs >= blinkIntervalMs / 2) {
    lastBlinkMs = now;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  }
}

/*
  RTC tester for the Arduino Uno + RTC + 4-digit TM1637 display.

  Wiring:
    TM1637 CLK -> D10
    TM1637 DIO -> D9
    TM1637 VCC -> 5V
    TM1637 GND -> GND
    RTC SCL    -> A5
    RTC SDA    -> A4
    RTC VCC    -> 5V
    RTC GND    -> GND

  Required libraries:
    - RTClib
    - TM1637

  The sketch tries DS1307 first, then DS3231.
  The display shows "group number : minute".
*/

#include <Wire.h>
#include <RTClib.h>
#include "src/TM1637/src/TM1637.h"

constexpr uint8_t TM1637_CLK_PIN = 10;
constexpr uint8_t TM1637_DIO_PIN = 9;
constexpr uint8_t DISPLAY_BRIGHTNESS = BRIGHT_TYPICAL;
constexpr uint8_t GROUP_NUMBER = 1;
constexpr unsigned long SERIAL_INTERVAL_MS = 2000;
constexpr unsigned long DISPLAY_INTERVAL_MS = 500;

RTC_DS1307 rtcDs1307;
RTC_DS3231 rtcDs3231;
TM1637 display(TM1637_CLK_PIN, TM1637_DIO_PIN);

enum class RtcType : uint8_t {
  None,
  DS1307,
  DS3231,
};

RtcType rtcType = RtcType::None;
unsigned long lastSerialMs = 0;
unsigned long lastDisplayMs = 0;
int lastDisplayedMinute = -1;

const char *rtcName() {
  switch (rtcType) {
    case RtcType::DS1307:
      return "DS1307";
    case RtcType::DS3231:
      return "DS3231";
    default:
      return "NONE";
  }
}

RTC_DS1307 &activeRtc() {
  return rtcDs1307;
}

bool detectRtc() {
  if (rtcDs1307.begin()) {
    rtcType = RtcType::DS1307;
    return true;
  }

  if (rtcDs3231.begin()) {
    rtcType = RtcType::DS3231;
    return true;
  }

  rtcType = RtcType::None;
  return false;
}

void showGroupAndMinute(int minute) {
  const int8_t digits[4] = {
      static_cast<int8_t>(GROUP_NUMBER % 10),
      0x7f,
      static_cast<int8_t>((minute / 10) % 10),
      static_cast<int8_t>(minute % 10),
  };

  display.point(POINT_OFF);
  display.display(const_cast<int8_t *>(digits));
}

DateTime readDateTime() {
  if (rtcType == RtcType::DS3231) {
    return rtcDs3231.now();
  }
  return rtcDs1307.now();
}

void printDateTime(const DateTime &now) {
  Serial.print(F("Group "));
  Serial.print(GROUP_NUMBER);
  Serial.print(F(" | RTC: "));
  Serial.print(rtcName());
  Serial.print(F(" | "));
  Serial.print(now.year());
  Serial.print('/');
  Serial.print(now.month());
  Serial.print('/');
  Serial.print(now.day());
  Serial.print(' ');

  if (now.hour() < 10) {
    Serial.print('0');
  }
  Serial.print(now.hour());
  Serial.print(':');
  if (now.minute() < 10) {
    Serial.print('0');
  }
  Serial.print(now.minute());
  Serial.print(':');
  if (now.second() < 10) {
    Serial.print('0');
  }
  Serial.println(now.second());
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  display.init();
  display.set(DISPLAY_BRIGHTNESS);
  display.point(POINT_OFF);
  display.clearDisplay();

  Serial.println(F("RTCtester starting"));
  Serial.print(F("Group "));
  Serial.println(GROUP_NUMBER);

  if (!detectRtc()) {
    Serial.println(F("RTC not found. Check SDA=A4, SCL=A5, VCC and GND."));
    return;
  }

  Serial.print(F("RTC detected: "));
  Serial.println(rtcName());

  const DateTime now = readDateTime();
  if (!now.isValid()) {
    Serial.println(F("RTC time is invalid. Check the coin cell."));
  }
}

void loop() {
  const unsigned long nowMs = millis();

  if (rtcType == RtcType::None) {
    if (nowMs - lastSerialMs >= SERIAL_INTERVAL_MS) {
      lastSerialMs = nowMs;
      Serial.println(F("Group 1 | RTC not found"));
      detectRtc();
    }
    return;
  }

  if (nowMs - lastDisplayMs >= DISPLAY_INTERVAL_MS) {
    lastDisplayMs = nowMs;

    const DateTime now = readDateTime();
    if (now.isValid() && now.minute() != lastDisplayedMinute) {
      lastDisplayedMinute = now.minute();
      showGroupAndMinute(now.minute());
    }
  }

  if (nowMs - lastSerialMs >= SERIAL_INTERVAL_MS) {
    lastSerialMs = nowMs;

    const DateTime now = readDateTime();
    if (now.isValid()) {
      printDateTime(now);
    } else {
      Serial.println(F("Group 1 | Invalid RTC time"));
    }
  }
}

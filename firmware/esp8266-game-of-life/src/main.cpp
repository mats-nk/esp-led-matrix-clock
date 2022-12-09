#include <Arduino.h>
#include <SPI.h>
#include <Ticker.h>

#include "LifeGame.h"
#include "ConsoleVisualizer.h"
#include "LedVisualizer.h"
#include "LedMatrix.h"

#ifdef ESP32
#include <WiFi.h>
#endif
#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif

#if defined(HW_SERIAL)
#define SERIAL_EN
#endif
#define TIME_PER_STEP_MS 500

LifeGame game;
Ticker tick;

#ifdef SERIAL_EN
ConsoleVisualizer *console = new ConsoleVisualizer(&game);
#endif
LedVisualizer *led = new LedVisualizer(&game);

void setup()
{
#ifdef SERIAL_EN
  Serial.begin(SERIAL_BAUD);
#if ARDUINO_HW_CDC_ON_BOOT
  delay(2000);
#else
  delay(100);
#endif
  Serial.println("\nStarted");
#endif

  WiFi.mode(WIFI_OFF);

  game.fillRandom();

  tick.attach_ms(TIME_PER_STEP_MS, []()
                 {
#ifdef SERIAL_EN
    console->display();
#endif
    led->display();

    game.doStep(); });

#ifdef SERIAL_EN
  Serial.println("Setup finished");
#endif
}

void loop()
{
}
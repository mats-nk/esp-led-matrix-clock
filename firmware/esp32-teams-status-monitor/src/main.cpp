#include <Arduino.h>
#include <SPI.h>

const String __loading_string_1 = "Connecting";
const String __loading_string_2 = "Sync time";
const String __loading_string_3 = "Check token";

#if defined(WIFI_SSID) && defined(WIFI_PASS)
#define WIFI_STATIC
#else
#include <WiFiManager.h>
WiFiManager wm;
#endif

#include <LedMatrix.h>
LedMatrix matrix(SCREEN_CNT);

#include "Connect.h"
Connect conn(&wm);

#include "TimeSource.h"
TimeSource timeSource;

#include "Loader.h"
Loader loader(&matrix);

#include "renderer/Clock.h"
Clock _clock(&matrix, &timeSource);

#include "renderer/AutoBrightness.h"
AutoBrightness Brightness(&matrix, &timeSource);

#include "TokenClient.h"
TokenClient token(&timeSource, APP_CLIENT_ID, APP_TENANT_ID);

#include "renderer/TeamsStatus.h"
TeamsStatus teams(&matrix, &token, &loader);

#include "renderer/Renderer.h"
Renderer *renderers[] = {
    &Brightness,
    // &_clock,
    &teams,
};

void setup()
{
    Serial.begin(SERIAL_BAUD);
#if ARDUINO_HW_CDC_ON_BOOT
    delay(2000);
#else
    delay(100);
#endif
    matrix.init();
    matrix.clear();

    loader.scroll(__loading_string_1);

    if (!conn.connect())
    {
        log_e("Failed to get online");
        delay(3000);
        ESP.restart();
    }

    loader.scroll(__loading_string_2);

    timeSource.init();
    timeSource.sync();
    
    loader.scroll(__loading_string_3);

    token.init();

    for (uint8_t i = 0; i < sizeof(renderers) / sizeof(Renderer *); i++)
        renderers[i]->init();

    log_d("Setup finished!");
}

void loop()
{
    for (uint8_t i = 0; i < sizeof(renderers) / sizeof(Renderer *); i++)
        renderers[i]->display();
}
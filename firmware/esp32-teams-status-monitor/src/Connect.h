#pragma once

#ifdef ESP32
#include <WiFi.h>
#endif
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#endif

#include <WiFiManager.h>

class Connect
{
private:
    WiFiManager *wm;

public:
    Connect(WiFiManager *wm)
        : wm(wm)
    {
        String ssid = "ESP-" + String((unsigned long)
#ifdef ESP8266
        ESP.getChipId()
#endif
#ifdef ESP32
        ESP.getEfuseMac()
#endif
        );
        WiFi.hostname(ssid);
    };

    bool connect();
    void disconnect();
};

bool Connect::connect()
{
    WiFi.mode(WIFI_STA);
#ifdef ARDUINO_LOLIN_C3_MINI
    WiFi.setTxPower(WIFI_POWER_8_5dBm); // https://github.com/tzapu/WiFiManager/issues/1422

#endif

#ifdef WIFI_STATIC
    uint8_t cnt = 0;
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        log_d(".");
        cnt++;
        if (cnt > 20)
            break;
    }

    if (WiFi.status() != WL_CONNECTED)
        return;
#else
    if (!wm->autoConnect())
    {
        log_e("Failed to connect and hit timeout");
#ifdef ESP8266
        ESP.reset();
#endif
#ifdef ESP32
        ESP.restart();
#endif
        return false;
    }
#endif
    log_i("Connected with: %s", WiFi.SSID().c_str());
    log_i("IP Address: %s", WiFi.localIP().toString());
    return true;
}

void Connect::disconnect()
{
    WiFi.mode(WIFI_OFF);
}
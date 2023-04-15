#pragma once

#include <Arduino.h>

#ifdef ESP32
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#endif
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#endif

#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

class JsonHttpClient
{
private:
    WiFiClientSecure client;
    HTTPClient http;
#ifdef ESP8266
    X509List *cert;
#endif
public:
    JsonHttpClient() {};

    void init(const char *rootCert);

    bool get(String url, int &httpCode, DynamicJsonDocument &doc, String authToken = "");
    bool post(String url, String content, int &httpCode, DynamicJsonDocument &doc);
};

void JsonHttpClient::init(const char *rootCert)
{
#ifdef ESP8266
    cert = new X509List(rootCert);
    client.setTrustAnchors(cert);
    // client.setFingerprint(fingerprint_stamp2_login_microsoftonline_com);
#endif
#ifdef ESP32
    client.setCACert(rootCert);
#endif
    // TODO: remove that
    client.setInsecure();
    client.setTimeout(3000);
}

bool JsonHttpClient::get(String url, int &httpCode, DynamicJsonDocument &doc, String authToken)
{
    log_d("GET %s", url.c_str());
    if (!http.begin(client, url))
    {
        log_e("Connection failed: %s", url);
        return false;
    }

    if (authToken != "")
    {
        http.addHeader("Authorization", "Bearer " + authToken);
    }
    httpCode = http.GET();

    // log_d("GET: %s", http.getString().c_str());

    // httpCode will be negative on error
    if (httpCode < 0)
    {
        log_e("GET %s ... failed, error: %s", url.c_str(), http.errorToString(httpCode).c_str());
        http.end();
        return false;
    }

    bool result = false;

    // HTTP header has been send and Server response header has been handled
    log_d("GET %s ... code: %d", url.c_str(), httpCode);

    uint8_t buf[16];
    uint8_t readBytes = http.getStream().readBytesUntil('\n', buf, 16);
    log_d("Bytes read: %d", readBytes);
    for (uint8_t i = 0; i < readBytes; i++)
    {
        Serial.printf("0x%X ", buf[i]);
    }

    // file found at server
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
    {
        DeserializationError error = deserializeJson(doc, http.getStream());

        // Test if parsing succeeds.
        if (error)
        {
            log_e("deserializeJson() failed: %s", error.f_str());
            log_d("<<<%s", http.getString().c_str());
            result = false;
        }
        else
        {
            result = true;
        }
    }

    http.end();
    return result;
}

bool JsonHttpClient::post(String url, String content, int &httpCode, DynamicJsonDocument &doc)
{
    log_d("POST %s", url.c_str());
    if (!http.begin(client, url))
    {
        log_e("Connection failed: %s", url);
        return false;
    }

    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    httpCode = http.POST(content);

    // log_d(http.getString());

    // httpCode will be negative on error
    if (httpCode < 0)
    {
        log_e("POST %s ... failed, error: %s", url.c_str(), http.errorToString(httpCode).c_str());
        http.end();
        return false;
    }

    bool result = false;

    // HTTP header has been send and Server response header has been handled
    log_d("POST %s ... code: %d", url.c_str(), httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
    {
        DeserializationError error = deserializeJson(doc, http.getStream());

        // Test if parsing succeeds.
        if (error)
        {
            log_e("deserializeJson() failed: %s", error.f_str());
            log_d("<<<%s", http.getString().c_str());
            result = false;
        }
        else
        {
            result = true;
        }
    }

    http.end();
    return result;
}
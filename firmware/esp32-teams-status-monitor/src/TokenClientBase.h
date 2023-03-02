#pragma once

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

#include <Ticker.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "Connect.h"
#include "JsonHttpClient.h"
#include "TimeSource.h"
#include "cert/login.microsoftonline.com.h"

#define CONTENT_LEN 1024 * 4

#define MAGIC_NUMBER 0x1234
#define USRCODE_LEN 16
#define ACCESS_TOKEN_LEN 1024 * 4
#define REFRESH_TOKEN_LEN 1024
#define DEVCODE_LEN 256

struct DeviceAuthResponse
{
    char user_code[USRCODE_LEN];
    char device_code[DEVCODE_LEN];
};

struct TokenClientSettings
{
    uint32_t magic;
    char refresh_token[REFRESH_TOKEN_LEN];
    char access_token[ACCESS_TOKEN_LEN];
    uint32_t expires_on;
};

enum TokenClientStatus
{
    INIT,
    AUTH_PENDING,
    AUTH_EXPIRED,
    AUTH_GRANTED,
    AUTHORIZED
};

class TokenClientBase
{
private:
    JsonHttpClient _client;

protected:
    const String clientId;
    const String tenantId;

    DeviceAuthResponse authResponse;

public:
    TokenClientSettings Settings;
    TimeSource *timeSource;
    Ticker tick;

    unsigned long auth_request_expires_on = 0;
    bool requestDeviceAuth();
    unsigned long auth_expires_on = 0;
    bool checkDeviceAuthRequest();
    bool requestToken();

    TokenClientStatus Status = INIT;

    TokenClientBase(TimeSource *timeSource, String clientId, String tenantId)
        : timeSource(timeSource), clientId(clientId), tenantId(tenantId)
    {
        _client.init(cert_DigiCert_Global_Root_CA);
    };
};

void __tokenclientbase_ticker_callback(TokenClientBase *self)
{
    // check token
    if (self->checkDeviceAuthRequest())
    {
        self->Status = AUTH_GRANTED;
        self->tick.detach();
    }
    else
    {
        self->Status = AUTH_PENDING;
    }

    // cancel if expired
    if (millis() >= self->auth_request_expires_on)
    {
        self->Status = AUTH_EXPIRED;
        self->tick.detach();
    }
}

bool TokenClientBase::requestDeviceAuth()
{
    String path = "/" + tenantId + "/oauth2/v2.0/devicecode";
    String fullUrl = "https://" + String(login_host) + String(path);
    String content = "client_id=" + clientId + "&scope=Presence.Read%20offline_access";
    int status;
    DynamicJsonDocument doc = DynamicJsonDocument(CONTENT_LEN);
    if (!_client.post(fullUrl, content, status, doc))
    {
        return false;
    }

    const char *code = doc["user_code"];
    const char *device_code = doc["device_code"];
    log_v("User code: %s", code);
    log_v("Device code: %s", device_code);
    const char *message = doc["message"];
    log_i("Prompt: %s", message);
    strcpy(authResponse.device_code, device_code);
    strcpy(authResponse.user_code, code);

    Status = AUTH_PENDING;

    int expires_in = doc["expires_in"];
    auth_request_expires_on = millis() + expires_in * 1000;
    log_d("AUTH| Now: %ld, expires on: %ld", millis(), auth_request_expires_on);
    int interval = doc["interval"];
    tick.attach(interval, __tokenclientbase_ticker_callback, this);
    return true;
}

bool TokenClientBase::checkDeviceAuthRequest()
{
    String path = "/" + tenantId + "/oauth2/v2.0/token";
    String fullUrl = "https://" + String(login_host) + String(path);
    String content = "client_id=" + String(APP_CLIENT_ID) + "&code=" + String(authResponse.device_code) + "&grant_type=urn:ietf:params:oauth:grant-type:device_code";
    int status;
    DynamicJsonDocument doc = DynamicJsonDocument(CONTENT_LEN);
    if (!_client.post(fullUrl, content, status, doc))
    {
        return false;
    }

    log_d("REF_TOKEN| %d", status);

    if (status == HTTP_CODE_OK)
    {
        const char *access_token = doc["access_token"];
        const char *refresh_token = doc["refresh_token"];
        int expires = doc["expires_in"];
        auth_expires_on = millis() + expires * 1000;

        log_d("REF_TOKEN| Now: %ld, expires on: %ld", millis(), auth_expires_on);
        log_v("access_token: %s", access_token);
        log_v("refresh_token: %s", refresh_token);

        strcpy(Settings.access_token, access_token);
        strcpy(Settings.refresh_token, refresh_token);
        Settings.expires_on = timeSource->getTime() + expires;

        return true;
    }
    else if (status == HTTP_CODE_BAD_REQUEST)
    {
        const char *error = doc["error"];
        log_w("REF_TOKEN| %d: %s", status, error);
        return false;
    }
    else
    {
        return false;
    }
}

bool TokenClientBase::requestToken()
{
    String path = "/" + tenantId + "/oauth2/v2.0/token";
    String fullUrl = "https://" + String(login_host) + String(path);
    String content = "client_id=" + String(APP_CLIENT_ID) + "&refresh_token=" + String(Settings.refresh_token) + "&grant_type=refresh_token";

    int status;
    DynamicJsonDocument doc = DynamicJsonDocument(CONTENT_LEN);
    if (!_client.post(fullUrl, content, status, doc))
    {
        return false;
    }

    log_d("AUTH_TOKEN| %d", status);

    if (status == HTTP_CODE_OK)
    {
        const char *access_token = doc["access_token"];
        const char *refresh_token = doc["refresh_token"];
        int expires = doc["expires_in"];
        auth_expires_on = millis() + expires * 1000;

        log_d("AUTH_TOKEN| Now: %ld, expires on: %ld", millis(), auth_expires_on);
        log_v("access_token: %s", access_token);
        log_v("refresh_token: %s", refresh_token);

        strcpy(Settings.access_token, access_token);
        strcpy(Settings.refresh_token, refresh_token);
        Settings.expires_on = timeSource->getTime() + expires;

        return true;
    }
    else if (status == HTTP_CODE_BAD_REQUEST)
    {
        const char *error = doc["error"];
        log_w("AUTH_TOKEN| %d: %s", status, error);
        return false;
    }
    else
    {
        return false;
    }
}
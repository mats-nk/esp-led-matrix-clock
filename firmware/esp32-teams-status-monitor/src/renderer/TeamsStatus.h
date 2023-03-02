#pragma once

#ifdef ESP32
#include <WiFi.h>
#endif
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#endif

#include <WiFiClientSecure.h>
#include <Ticker.h>

#include "Renderer.h"
#include "../TokenClient.h"
#include "../Loader.h"
#include "../cert/graph.microsoft.com.h"

#define PRESENCE_CONTENT_LEN 512
const String __loading_string_4 = "Request presence";

class TeamsStatus : public Renderer
{
private:
    const unsigned short _maxPosX = SCREEN_CNT * 8 - 1;
    unsigned int _dPosX = _maxPosX;

public:
    Ticker tick;
    JsonHttpClient _client;
    TokenClient *token;
    Loader *loader;

    String availability = "N/A";
    String activity = "N/A";
    bool gotStatus = false;

    TeamsStatus(LedMatrix *mx, TokenClient *token, Loader *loader)
        : Renderer(mx), token(token), loader(loader)
    {
        _client.init(cert_DigiCert_Global_Root_CA);
    };

    void init() override;
    void display() override;
};

void __teamsstatus_ticker_callback(TeamsStatus *self)
{
    if (self->token->Status == AUTHORIZED)
    {
        String token = self->token->get();
        String path = "/v1.0/me/presence";
        String fullUrl = "https://" + String(graph_host) + String(path);

        int status;
        DynamicJsonDocument doc = DynamicJsonDocument(PRESENCE_CONTENT_LEN);
        if (!self->_client.get(fullUrl, status, doc, token))
        {
            log_e("GRAPH call failed: %d", status);
        }
        else
        {
            self->gotStatus = true;
            String availability = doc["availability"].as<String>();
            self->availability = availability;
            log_i("availability = %s, activity = %s\n", self->availability, self->activity);

            String activity = doc["activity"].as<String>();
            if (self->activity != activity)
            {
                self->activity = activity;
                self->loader->scroll(self->activity);
            }
        }
    }
    else
    {
        log_w("Token is not ready yet, status: %d", self->token->Status);
    }

    self->tick.once(5, __teamsstatus_ticker_callback, self);
}

void TeamsStatus::init()
{
    tick.once(5, __teamsstatus_ticker_callback, this);
    loader->scroll(__loading_string_4);
}

void TeamsStatus::display()
{
    // if (gotStatus)
    // {
    //     loader
    //     mx->scrollText(_dPosX++, activity);
    //     mx->apply();
    // }
}
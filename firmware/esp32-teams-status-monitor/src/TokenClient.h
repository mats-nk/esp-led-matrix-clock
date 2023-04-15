#pragma once

#include <Ticker.h>
#include <EEPROM.h>
#include "TokenClientBase.h"

#define STATUS_CHECK_SEC 5

class TokenClient : public TokenClientBase
{
private:
protected:
    void getRefreshToken();

public:
    Ticker tick;
    bool load();
    bool save();

    TokenClient(TimeSource *timeSource, String clientId, String tenantId)
        : TokenClientBase(timeSource, clientId, tenantId){};

    void init();
    String get();
};

bool TokenClient::load()
{
    EEPROM.get(0, Settings);
    return Settings.magic == MAGIC_NUMBER;
}

String TokenClient::get()
{
    // TODO: check and refresh if expired
    return Settings.access_token;
}

bool TokenClient::save()
{
    Settings.magic = MAGIC_NUMBER;
    EEPROM.put(0, Settings);

    if (EEPROM.commit())
    {
        log_d("EEPROM successfully committed");
        return true;
    }
    else
    {
        log_e("ERROR! EEPROM commit failed");
        return false;
    }
}

void __tokenclient_ticker_callback(TokenClient *self)
{
    log_d("STATUS = %d", self->Status);

    switch (self->Status)
    {
    case INIT:
    {
        if (!self->load())
        {
            if (!self->requestDeviceAuth())
            {
                log_e("requestDeviceAuth FAILED!");
            }
        }
        else
        {
            // TODO: validate if refresh_token is valid
            time_t _expires = self->Settings.expires_on;
            auto expires = localtime(&_expires);
            log_d("access_token expires on [%ld]%s", _expires, asctime(expires));
            if (self->Settings.expires_on < self->timeSource->getTime())
            {
                self->Status = AUTH_EXPIRED;
            }
            else
            {
                self->Status = AUTHORIZED;
            }
        }
    }
    break;

    case AUTH_GRANTED:
    {
        if (!self->save())
        {
            log_e("Token store FAILED!");
        }
        self->Status = AUTHORIZED;
    }
    break;

    case AUTH_EXPIRED:
        if (!self->requestToken())
        {
            log_e("requestToken FAILED!");
        }
        else
        {
            self->Status = AUTH_GRANTED;
        }
        break;

    case AUTHORIZED:
    default:
        break;
    }
}

void TokenClient::init()
{
    if (!EEPROM.begin(sizeof(TokenClientSettings)))
    {
        log_e("EEPROM begin failed!");
    }
    tick.attach(STATUS_CHECK_SEC, __tokenclient_ticker_callback, this);
}

#pragma once

#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#endif
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#endif

#include <Ticker.h>
#include <time.h>
#include "tz.h"

#include "Connect.h"

#define NTP_SERVERS "us.pool.ntp.org", "time.nist.gov", "pool.ntp.org"
#define NTP_MIN_VALID_EPOCH 1533081600 // August 1st, 2018

#define NTP_UPDATE_EVERY_SEC (60 * 60 * 24)

class TimeSource
{
private:
    Connect *conn;
    Ticker tick;
    tm Time;

public:
    TimeSource(): conn(nullptr){};
    TimeSource(Connect *conn) : conn(conn){};
    ~TimeSource();

    void init();
    void sync();

    tm* get();
    time_t getTime();
};

tm* TimeSource::get()
{
    time_t now = time(&now);
    localtime_r(&now, &Time);
    return &Time;
}

time_t TimeSource::getTime() {
    return mktime(get());
}

TimeSource::~TimeSource()
{
    tick.detach();
}

void __timesource_ticker_callback(TimeSource *self)
{
    self->sync();
}

void TimeSource::init()
{
    tick.attach(NTP_UPDATE_EVERY_SEC, __timesource_ticker_callback, this);
}

void TimeSource::sync()
{
    if (conn == nullptr || conn->connect())
    {
        time_t now;

#ifdef ESP8266
        configTime(TIMEZONE, NTP_SERVERS);
#endif
#ifdef ESP32
        configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVERS);
#endif

        log_d("Requesting current time ");
        int i = 1;
        while ((now = time(nullptr)) < NTP_MIN_VALID_EPOCH)
        {
            log_d(".");
            delay(300);
            yield();
            i++;
        }
        log_d("Time synchronized");
        log_i("Local time: %s", asctime(localtime(&now))); // print formated local time, same as ctime(&now)
        log_i("UTC time:   %s", asctime(gmtime(&now)));    // print formated GMT/UTC time

        if (conn != nullptr)
            conn->disconnect();
    }
    else
    {
        log_e("Failed to get online");
    }
}

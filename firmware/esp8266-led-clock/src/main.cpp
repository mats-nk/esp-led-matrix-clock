//*********************************************************************************************************
//*    ESP8266 MatrixClock                                                                                *
//*********************************************************************************************************
//
// first release on 26.02.2017
// updated on    30.11.2020
// Version 2.0.2
//
//
// THE SOFTWARE IS PROVIDED "AS IS" FOR PRIVATE USE ONLY, IT IS NOT FOR COMMERCIAL USE IN WHOLE OR PART OR CONCEPT.
// FOR PERSONAL USE IT IS SUPPLIED WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHOR
// OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE
//
//

#include <Arduino.h>
#include <SPI.h>
#include <Ticker.h>
#ifdef ESP32
#include <WiFi.h>
#endif
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#endif

#if defined(WIFI_SSID) && defined(WIFI_PASS)
#define WIFI_STATIC
#else
#include <WiFiManager.h>
WiFiManager _wifiManager;
#endif

#include <time.h>
#include <LedMatrix.h>
#include "fonts.h"
#include "tz.h"

struct tm tm; // http://www.cplusplus.com/reference/ctime/tm/
LedMatrix *matrix = new LedMatrix(SCREEN_CNT);

unsigned short _maxPosX = SCREEN_CNT * 8 - 1; // calculated maxpos
unsigned int _zPosX = 0;                      // xPos for time
unsigned int _dPosX = _maxPosX;               // xPos for date
bool _f_tckr50ms = false;                     // flag, set every 50msec
boolean _f_updown = false;                    // scroll direction

#define SCREEN_MIN_BRIGHTNESS 0x01
#define SCREEN_MAX_BRIGHTNESS 0x0b
unsigned short screen_current_brightness = SCREEN_MIN_BRIGHTNESS;
unsigned short screen_desired_brightness = SCREEN_MIN_BRIGHTNESS;

#define NTP_SERVERS "us.pool.ntp.org", "time.nist.gov", "pool.ntp.org"
#define NTP_MIN_VALID_EPOCH 1533081600 // August 1st, 2018

// The object for the Ticker
Ticker tckr;

String M_arr[12] = {"Jan.", "Feb.", "Mar.", "Apr.", "May", "June", "July", "Aug.", "Sep.", "Oct.", "Nov.", "Dec."};
String WD_arr[7] = {"Sun,", "Mon,", "Tue,", "Wed,", "Thu,", "Fri,", "Sat,"};

//----------------------------------------------------------------------------------------------------------------------

extern "C" uint8_t sntp_getreachability(uint8_t);

void syncTime()
{
    // connect WiFi -> fetch ntp packet -> disconnect Wifi
    // uint8_t cnt = 0;

    WiFi.mode(WIFI_STA);
// https://github.com/tzapu/WiFiManager/issues/1422
#ifdef ARDUINO_LOLIN_C3_MINI
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
#endif

#ifdef WIFI_STATIC
    uint8_t cnt = 0;
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        cnt++;
        if (cnt > 20)
            break;
    }

    if (WiFi.status() != WL_CONNECTED)
        return;
#else
    if (!_wifiManager.autoConnect())
    {
        Serial.println("Failed to connect and hit timeout");
#ifdef ESP8266
        ESP.reset();
#endif
#ifdef ESP32
        ESP.restart();
#endif
    }
#endif

    Serial.println("\nConnected with: " + WiFi.SSID());
    Serial.println("IP Address: " + WiFi.localIP().toString());

    time_t now;

#ifdef ESP8266
    configTime(TIMEZONE, NTP_SERVERS);
#endif
#ifdef ESP32
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVERS);
#endif

    Serial.print("Requesting current time ");
    int i = 1;
    while ((now = time(nullptr)) < NTP_MIN_VALID_EPOCH)
    {
        Serial.print(".");
        delay(300);
        yield();
        i++;
    }
    Serial.println("Time synchronized");
    Serial.printf("Local time: %s", asctime(localtime(&now))); // print formated local time, same as ctime(&now)
    Serial.printf("UTC time:   %s", asctime(gmtime(&now)));    // print formated GMT/UTC time

    WiFi.mode(WIFI_OFF);
}

// detect desired screen brightness based on hour
unsigned short max7219_calc_brightness(unsigned short hour)
{
    if (hour >= 20)
        return SCREEN_MIN_BRIGHTNESS;
    else if (hour >= 16)
        return map(hour, 16, 20, SCREEN_MAX_BRIGHTNESS, SCREEN_MIN_BRIGHTNESS);
    else if (hour >= 8)
        return SCREEN_MAX_BRIGHTNESS;
    else if (hour >= 4)
        return map(hour, 4, 8, SCREEN_MIN_BRIGHTNESS, SCREEN_MAX_BRIGHTNESS);
    else
        return SCREEN_MIN_BRIGHTNESS;
}

void max7219_update_brightness(unsigned short hour)
{
    screen_desired_brightness = max7219_calc_brightness(hour);
    if (screen_current_brightness > screen_desired_brightness)
        matrix->setBrightness(--screen_current_brightness);
    else if (screen_current_brightness < screen_desired_brightness)
        matrix->setBrightness(++screen_current_brightness);
    // Serial.printf("%d -> %d <- %d\n", hour, screen_desired_brightness, screen_current_brightness);
}

void setup()
{
    Serial.begin(SERIAL_BAUD);
#if ARDUINO_HW_CDC_ON_BOOT
    delay(2000);
#else
    delay(100);
#endif

    matrix->init();
    matrix->clear();
    matrix->setBrightness(SCREEN_MIN_BRIGHTNESS);

    WiFi.mode(WIFI_OFF);

    String ssid = "ESP-" + String((unsigned long)
#ifdef ESP8266
        ESP.getChipId()
#endif
#ifdef ESP32
        ESP.getEfuseMac()
#endif
    );
    WiFi.hostname(ssid);

    tckr.attach(0.1, [](){
        matrix->scrollText(_dPosX++, "Loading. .  .   .    .");
        matrix->apply();
    });
    syncTime();
    tckr.detach();
    matrix->clear();
    
    tckr.attach(0.05, [](){
        static unsigned int cnt50ms = 0;
        static unsigned int cnt1s = 0;
        static unsigned int cnt1h = 0;
        _f_tckr50ms = true;
        cnt50ms++;
        if (cnt50ms == 20)
        {
            cnt1s++;
            cnt50ms = 0;
        }
        if (cnt1s == 3600)
        { // 1h
            cnt1h++;
            cnt1s = 0;
        }
        if (cnt1h == 24)
        { // 1d
            syncTime();
            cnt1h = 0;
        } 
    }); // every 50 msec

#ifdef SCREEN_SCROLLDOWN
    _f_updown = true;
#else
    _f_updown = false;
#endif

    _zPosX = _maxPosX;
    _dPosX = -8;

    // refresh_display();
    matrix->apply();
}

void loop()
{
    static uint8_t sec1 = 0, sec2 = 0, min1 = 0, min2 = 0, hour1 = 0, hour2 = 0;
    static uint8_t sec11 = 0, sec12 = 0, sec21 = 0, sec22 = 0;
    static uint8_t min11 = 0, min12 = 0, min21 = 0, min22 = 0;
    static uint8_t hour11 = 0, hour12 = 0, hour21 = 0, hour22 = 0;
    static time_t lastsec{0};
    static signed int x = 0; // x1,x2;
    static signed int y = 0, y1 = 0, y2 = 0;
    static unsigned int sc1 = 0, sc2 = 0, sc3 = 0, sc4 = 0, sc5 = 0, sc6 = 0;
    static uint16_t sctxtlen = 0;
    static boolean f_scrollend_y = false;
    static boolean f_scroll_x1 = false;
    static boolean f_scroll_x2 = false;

    time_t now = time(&now);
    localtime_r(&now, &tm);

    if (_f_updown == false)
    {
        y2 = -9;
        y1 = 8;
    }

    if (_f_updown == true)
    { // scroll  up to down
        y2 = 8;
        y1 = -8;
    }

    if (tm.tm_sec != lastsec)
    {
        lastsec = tm.tm_sec;

        sec1 = (tm.tm_sec % 10);
        sec2 = (tm.tm_sec / 10);
        min1 = (tm.tm_min % 10);
        min2 = (tm.tm_min / 10);
#ifdef SCREEN_FORMAT24H
        hour1 = (tm.tm_hour % 10); // 24 hour format
        hour2 = (tm.tm_hour / 10);
#else
        uint8_t h = tm.tm_hour; // convert to 12 hour format
        if (h > 12)
            h -= 12;
        if (h == 0)
            h = 12;
        hour1 = (h % 10);
        hour2 = (h / 10);
#endif

        y = y2; // scroll updown
        sc1 = 1;
        sec1++;
        if (sec1 == 10)
        {
            sc2 = 1;
            sec2++;
            sec1 = 0;
        }
        if (sec2 == 6)
        {
            min1++;
            sec2 = 0;
            sc3 = 1;
        }
        if (min1 == 10)
        {
            min2++;
            min1 = 0;
            sc4 = 1;
        }
        if (min2 == 6)
        {
            hour1++;
            min2 = 0;
            sc5 = 1;
        }
        if (hour1 == 10)
        {
            hour2++;
            hour1 = 0;
            sc6 = 1;
        }
#ifdef SCREEN_FORMAT24H
        if ((hour2 == 2) && (hour1 == 4))
        {
            hour1 = 0;
            hour2 = 0;
            sc6 = 1;
        }
#else
        if ((hour2 == 1) && (hour1 == 3))
        { // 12 hour format
            hour1 = 1;
            hour2 = 0;
            sc6 = 1;
        }
#endif
        sec11 = sec12;
        sec12 = sec1;
        sec21 = sec22;
        sec22 = sec2;
        min11 = min12;
        min12 = min1;
        min21 = min22;
        min22 = min2;
        hour11 = hour12;
        hour12 = hour1;
        hour21 = hour22;
        hour22 = hour2;
        if (tm.tm_sec == 45)
            f_scroll_x1 = true; // scroll ddmmyy
#ifdef UDTXT
        if (tm.tm_sec == 25)
            f_scroll_x2 = true; // scroll userdefined text
#endif

        max7219_update_brightness(tm.tm_hour);
    }

    if (_f_tckr50ms == true)
    {
        _f_tckr50ms = false;
        // -------------------------------------
        if (f_scroll_x1 == true)
        {
            _zPosX++;
            _dPosX++;
            if (_dPosX == sctxtlen)
                _zPosX = 0;
            if (_zPosX == _maxPosX)
            {
                f_scroll_x1 = false;
                _dPosX = -8;
            }
        }
        // -------------------------------------
        if (f_scroll_x2 == true)
        {
            _zPosX++;
            _dPosX++;
            if (_dPosX == sctxtlen)
                _zPosX = 0;
            if (_zPosX == _maxPosX)
            {
                f_scroll_x2 = false;
                _dPosX = -8;
            }
        }

#if (SCREEN_CNT > 4)
        if (sc1 == 1)
        {
            if (_f_updown == 1)
                y--;
            else
                y++;
            matrix->char2Arr_t(48 + sec12, _zPosX - 42, y);
            matrix->char2Arr_t(48 + sec11, _zPosX - 42, y + y1);
            if (y == 0)
            {
                sc1 = 0;
                f_scrollend_y = true;
            }
        }
        else
            matrix->char2Arr_t(48 + sec1, _zPosX - 42, 0);

        if (sc2 == 1)
        {
            matrix->char2Arr_t(48 + sec22, _zPosX - 36, y);
            matrix->char2Arr_t(48 + sec21, _zPosX - 36, y + y1);
            if (y == 0)
                sc2 = 0;
        }
        else
            matrix->char2Arr_t(48 + sec2, _zPosX - 36, 0);

        matrix->char2Arr_t(':', _zPosX - 32, 0);
#endif

        if (sc3 == 1)
        {
            matrix->char2Arr_t(48 + min12, _zPosX - 25, y);
            matrix->char2Arr_t(48 + min11, _zPosX - 25, y + y1);
            if (y == 0)
                sc3 = 0;
        }
        else
            matrix->char2Arr_t(48 + min1, _zPosX - 25, 0);

        if (sc4 == 1)
        {
            matrix->char2Arr_t(48 + min22, _zPosX - 19, y);
            matrix->char2Arr_t(48 + min21, _zPosX - 19, y + y1);
            if (y == 0)
                sc4 = 0;
        }
        else
            matrix->char2Arr_t(48 + min2, _zPosX - 19, 0);

        matrix->char2Arr_t(':', _zPosX - 15 + x, 0);

        if (sc5 == 1)
        {
            matrix->char2Arr_t(48 + hour12, _zPosX - 8, y);
            matrix->char2Arr_t(48 + hour11, _zPosX - 8, y + y1);
            if (y == 0)
                sc5 = 0;
        }
        else
            matrix->char2Arr_t(48 + hour1, _zPosX - 8, 0);

        if (sc6 == 1)
        {
            matrix->char2Arr_t(48 + hour22, _zPosX - 2, y);
            matrix->char2Arr_t(48 + hour21, _zPosX - 2, y + y1);
            if (y == 0)
                sc6 = 0;
        }
        else
            matrix->char2Arr_t(48 + hour2, _zPosX - 2, 0);

        if (f_scroll_x1)
        { // day month year
            String txt = "   ";
            txt += WD_arr[tm.tm_wday] + " ";
            txt += String(tm.tm_mday) + ". ";
            txt += M_arr[tm.tm_mon] + " ";
            // txt += String(tm.tm_year + 1900) + "   ";
            sctxtlen = matrix->scrollText(_dPosX, txt);
        }
        //      -------------------------------------
        if (f_scroll_x2)
        { // user defined text
#ifdef UDTXT
            sctxtlen = scrolltext(_dPosX, UDTXT);
#endif // UDTXT
        }
        
        matrix->apply();
        if (f_scrollend_y == true)
            f_scrollend_y = false;
    } // end 50ms

    // -----------------------------------------------
    if (y == 0)
    {
        // do something else
    }
}
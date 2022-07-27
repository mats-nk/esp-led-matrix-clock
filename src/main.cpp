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
#include <WiFiManager.h>
WiFiManager _wifiManager;

#include <time.h>
#include "fonts.h"
#include "tz.h"

struct tm tm; // http://www.cplusplus.com/reference/ctime/tm/

unsigned short _maxPosX = SCREEN_CNT * 8 - 1; // calculated maxpos
unsigned short _LEDarr[SCREEN_CNT][8];        // character matrix to display (40*8)
unsigned short _helpArrMAX[SCREEN_CNT * 8];   // helperarray for chardecoding
unsigned short _helpArrPos[SCREEN_CNT * 8];   // helperarray pos of chardecoding
unsigned int _zPosX = 0;                      // xPos for time
unsigned int _dPosX = 0;                      // xPos for date
bool _f_tckr50ms = false;                     // flag, set every 50msec
boolean _f_updown = false;                    // scroll direction
uint16_t _chbuf[256];

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
    else
    {
        Serial.println("\nConnected with: " + WiFi.SSID());
        Serial.println("IP Address: " + WiFi.localIP().toString());
    }

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
//----------------------------------------------------------------------------------------------------------------------

const uint8_t InitArr[7][2] =
    {
        {0x0C, 0x00}, // display off
        {0x00, 0xFF}, // no LEDtest
        {0x09, 0x00}, // BCD off
        {0x0F, 0x00}, // normal operation
        {0x0B, 0x07}, // start display
        {0x0A, 0x04}, // brightness
        {0x0C, 0x01}  // display on
};

//----------------------------------------------------------------------------------------------------------------------

void helpArr_init(void)
{ // helperarray init
    uint8_t i, j, k;
    j = 0;
    k = 0;
    for (i = 0; i < SCREEN_CNT * 8; i++)
    {
        _helpArrPos[i] = (1 << j); // bitmask
        _helpArrMAX[i] = k;
        j++;
        if (j > 7)
        {
            j = 0;
            k++;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

void max7219_init()
{ // all MAX7219 init
    uint8_t i, j;
    for (i = 0; i < 7; i++)
    {
        digitalWrite(SCREEN_CS, LOW);
        delayMicroseconds(1);
        for (j = 0; j < SCREEN_CNT; j++)
        {
            SPI.write(InitArr[i][0]); // register
            SPI.write(InitArr[i][1]); // value
        }
        digitalWrite(SCREEN_CS, HIGH);
    }
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

void max7219_set_brightness(unsigned short br)
{ // brightness MAX7219
    uint8_t j;
    if (br < 16)
    {
        digitalWrite(SCREEN_CS, LOW);
        delayMicroseconds(1);
        for (j = 0; j < SCREEN_CNT; j++)
        {
            SPI.write(0x0A); // register
            SPI.write(br);   // value
        }
        digitalWrite(SCREEN_CS, HIGH);
    }
}

void max7219_update_brightness(unsigned short hour)
{
    screen_desired_brightness = max7219_calc_brightness(hour);
    if (screen_current_brightness > screen_desired_brightness)
        max7219_set_brightness(--screen_current_brightness);
    else if (screen_current_brightness < screen_desired_brightness)
        max7219_set_brightness(++screen_current_brightness);

    // Serial.printf("%d -> %d <- %d\n", hour, screen_desired_brightness, screen_current_brightness);
}

//----------------------------------------------------------------------------------------------------------------------

void clear_Display()
{ // clear all
    uint8_t i, j;
    for (i = 0; i < 8; i++)
    { // 8 rows
        digitalWrite(SCREEN_CS, LOW);
        delayMicroseconds(1);
        for (j = SCREEN_CNT; j > 0; j--)
        {
            _LEDarr[j - 1][i] = 0; // LEDarr clear
            SPI.write(i + 1);      // current row
            SPI.write(_LEDarr[j - 1][i]);
        }
        digitalWrite(SCREEN_CS, HIGH);
    }
}
//----------------------------------------------------------------------------------------------------------------------

void rotate_90()
{ // for Generic displays
    for (uint8_t k = SCREEN_CNT; k > 0; k--)
    {
        uint8_t i, j, m, imask, jmask;
        uint8_t tmp[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        for (i = 0, imask = 0x01; i < 8; i++, imask <<= 1)
        {
            for (j = 0, jmask = 0x01; j < 8; j++, jmask <<= 1)
            {
                if (_LEDarr[k - 1][i] & jmask)
                {
                    tmp[j] |= imask;
                }
            }
        }
        for (m = 0; m < 8; m++)
        {
            _LEDarr[k - 1][m] = tmp[m];
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------

void refresh_display()
{ // take info into LEDarr
    uint8_t i, j;
#ifdef ROTATE_90
    rotate_90();
#endif
    for (i = 0; i < 8; i++)
    { // 8 rows
        digitalWrite(SCREEN_CS, LOW);
        delayMicroseconds(1);
        for (j = SCREEN_CNT; j > 0; j--)
        {
            SPI.write(i + 1); // current row
#ifdef REVERSE_HORIZONTAL
            SPI.setBitOrder(LSBFIRST); // bitorder for reverse columns
#endif

#ifdef REVERSE_VERTICAL
            SPI.write(_LEDarr[j - 1][7 - i]);
#else
            SPI.write(_LEDarr[j - 1][i]);
#endif

#ifdef REVERSE_HORIZONTAL
            SPI.setBitOrder(MSBFIRST); // reset bitorder
#endif
        }
        digitalWrite(SCREEN_CS, HIGH);
    }
}
//----------------------------------------------------------------------------------------------------------------------

uint8_t char2Arr_t(unsigned short ch, int PosX, short PosY)
{ // characters into arr, shows only the time
    int i, j, k, l, m, o1, o2, o3, o4 = 0;
    PosX++;
    k = ch - 0x30; // ASCII position in font
    if ((k >= 0) && (k < 11))
    {                      // character found in font?
        o4 = font_t[k][0]; // character width
        o3 = 1 << (o4 - 1);
        for (i = 0; i < o4; i++)
        {
            if (((PosX - i <= _maxPosX) && (PosX - i >= 0)) && ((PosY > -8) && (PosY < 8)))
            { // within matrix?
                o1 = _helpArrPos[PosX - i];
                o2 = _helpArrMAX[PosX - i];
                for (j = 0; j < 8; j++)
                {
                    if (((PosY >= 0) && (PosY <= j)) || ((PosY < 0) && (j < PosY + 8)))
                    { // scroll vertical
                        l = font_t[k][j + 1];
                        m = (l & (o3 >> i)); // e.g. o4=7  0zzzzz0, o4=4  0zz0
                        if (m > 0)
                            _LEDarr[o2][j - PosY] = _LEDarr[o2][j - PosY] | (o1); // set point
                        else
                            _LEDarr[o2][j - PosY] = _LEDarr[o2][j - PosY] & (~o1); // clear point
                    }
                }
            }
        }
    }
    return o4;
}
//----------------------------------------------------------------------------------------------------------------------

uint8_t char2Arr_p(uint16_t ch, int PosX)
{ // characters into arr, proportional font
    int i, j, l, m, o1, o2, o3, o4 = 0;
    if (ch <= 345)
    {                       // character found in font?
        o4 = font_p[ch][0]; // character width
        o3 = 1 << (o4 - 1);
        for (i = 0; i < o4; i++)
        {
            if ((PosX - i <= _maxPosX) && (PosX - i >= 0))
            { // within matrix?
                o1 = _helpArrPos[PosX - i];
                o2 = _helpArrMAX[PosX - i];
                for (j = 0; j < 8; j++)
                {
                    l = font_p[ch][j + 1];
                    m = (l & (o3 >> i)); // e.g. o4=7  0zzzzz0, o4=4  0zz0
                    if (m > 0)
                        _LEDarr[o2][j] = _LEDarr[o2][j] | (o1); // set point
                    else
                        _LEDarr[o2][j] = _LEDarr[o2][j] & (~o1); // clear point
                }
            }
        }
    }
    return o4;
}
//----------------------------------------------------------------------------------------------------------------------

uint16_t scrolltext(int16_t posX, String txt)
{
    uint16_t i = 0, j = 0;
    boolean k = false;
    while ((txt[i] != 0) && (j < 256))
    {
        if ((txt[i] >= 0x20) && (txt[i] <= 0x7f))
        { // ASCII section
            _chbuf[j] = txt[i] - 0x20;
            k = true;
            i++;
            j++;
        }
        if (txt[i] == 0xC2)
        { // basic latin section (0x80...0x9f are controls, not used)
            if ((txt[i + 1] >= 0xA0) && (txt[i + 1] <= 0xBF))
            {
                _chbuf[j] = txt[i + 1] - 0x40;
                k = true;
                i += 2;
                j++;
            }
        }
        if (txt[i] == 0xC3)
        { // latin1 supplement section
            if ((txt[i + 1] >= 0x80) && (txt[i + 1] <= 0xBF))
            {
                _chbuf[j] = txt[i + 1] + 0x00;
                k = true;
                i += 2;
                j++;
            }
        }
        if (txt[i] == 0xCE)
        { // greek section
            if ((txt[i + 1] >= 0x91) && (txt[i + 1] <= 0xBF))
            {
                _chbuf[j] = txt[i + 1] + 0x2F;
                k = true;
                i += 2;
                j++;
            }
        }
        if (txt[i] == 0xCF)
        { // greek section
            if ((txt[i + 1] >= 0x80) && (txt[i + 1] <= 0x89))
            {
                _chbuf[j] = txt[i + 1] + 0x6F;
                k = true;
                i += 2;
                j++;
            }
        }
        if (txt[i] == 0xD0)
        { // cyrillic section
            if ((txt[i + 1] >= 0x80) && (txt[i + 1] <= 0xBF))
            {
                _chbuf[j] = txt[i + 1] + 0x79;
                k = true;
                i += 2;
                j++;
            }
        }
        if (txt[i] == 0xD1)
        { // cyrillic section
            if ((txt[i + 1] >= 0x80) && (txt[i + 1] <= 0x9F))
            {
                _chbuf[j] = txt[i + 1] + 0xB9;
                k = true;
                i += 2;
                j++;
            }
        }
        if (k == false)
        {
            _chbuf[j] = 0x00; // space 1px
            i++;
            j++;
        }
        k = false;
    }
    //  _chbuf stores the position of th char in font and in j is the length of the real string

    int16_t p = 0;
    for (int k = 0; k < j; k++)
    {
        p += char2Arr_p(_chbuf[k], posX - p);
        p += char2Arr_p(0, posX - p); // 1px space
        if (_chbuf[k] == 0)
            p += 2; // +2px space
    }
    return p;
}
//----------------------------------------------------------------------------------------------------------------------

void timer50ms()
{

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
}

//----------------------------------------------------------------------------------------------------------------------

void setup()
{
    Serial.begin(SERIAL_BAUD);
    #if ARDUINO_HW_CDC_ON_BOOT
    delay(2000);
    #else
    delay(100);
    #endif

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

    pinMode(SCREEN_CS, OUTPUT);
    digitalWrite(SCREEN_CS, HIGH);

    syncTime();

#ifdef ESP8266
    SPI.begin();
#endif
#ifdef ESP32
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
#endif
    helpArr_init();
    max7219_init();
    clear_Display();

    max7219_set_brightness(SCREEN_MIN_BRIGHTNESS);
    tckr.attach(0.05, timer50ms); // every 50 msec

#ifdef SCREEN_SCROLLDOWN
    _f_updown = true;
#else
    _f_updown = false;
#endif

    _zPosX = _maxPosX;
    _dPosX = -8;

    refresh_display();
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
            char2Arr_t(48 + sec12, _zPosX - 42, y);
            char2Arr_t(48 + sec11, _zPosX - 42, y + y1);
            if (y == 0)
            {
                sc1 = 0;
                f_scrollend_y = true;
            }
        }
        else
            char2Arr_t(48 + sec1, _zPosX - 42, 0);

        if (sc2 == 1)
        {
            char2Arr_t(48 + sec22, _zPosX - 36, y);
            char2Arr_t(48 + sec21, _zPosX - 36, y + y1);
            if (y == 0)
                sc2 = 0;
        }
        else
            char2Arr_t(48 + sec2, _zPosX - 36, 0);

        char2Arr_t(':', _zPosX - 32, 0);
        #endif
        
        if (sc3 == 1)
        {
            char2Arr_t(48 + min12, _zPosX - 25, y);
            char2Arr_t(48 + min11, _zPosX - 25, y + y1);
            if (y == 0)
                sc3 = 0;
        }
        else
            char2Arr_t(48 + min1, _zPosX - 25, 0);

        if (sc4 == 1)
        {
            char2Arr_t(48 + min22, _zPosX - 19, y);
            char2Arr_t(48 + min21, _zPosX - 19, y + y1);
            if (y == 0)
                sc4 = 0;
        }
        else
            char2Arr_t(48 + min2, _zPosX - 19, 0);

        char2Arr_t(':', _zPosX - 15 + x, 0);

        if (sc5 == 1)
        {
            char2Arr_t(48 + hour12, _zPosX - 8, y);
            char2Arr_t(48 + hour11, _zPosX - 8, y + y1);
            if (y == 0)
                sc5 = 0;
        }
        else
            char2Arr_t(48 + hour1, _zPosX - 8, 0);

        if (sc6 == 1)
        {
            char2Arr_t(48 + hour22, _zPosX - 2, y);
            char2Arr_t(48 + hour21, _zPosX - 2, y + y1);
            if (y == 0)
                sc6 = 0;
        }
        else
            char2Arr_t(48 + hour2, _zPosX - 2, 0);

        if (f_scroll_x1)
        { // day month year
            String txt = "   ";
            txt += WD_arr[tm.tm_wday] + " ";
            txt += String(tm.tm_mday) + ". ";
            txt += M_arr[tm.tm_mon] + " ";
            txt += String(tm.tm_year + 1900) + "   ";
            sctxtlen = scrolltext(_dPosX, txt);
        }
        //      -------------------------------------
        if (f_scroll_x2)
        { // user defined text
#ifdef UDTXT
            sctxtlen = scrolltext(_dPosX, UDTXT);
#endif // UDTXT
        }
        refresh_display(); // all 50msec
        if (f_scrollend_y == true)
            f_scrollend_y = false;
    } // end 50ms
    
    // -----------------------------------------------
    if (y == 0)
    {
        // do something else
    }
}
#ifndef WIFI_TIME_LIB_H
#define WIFI_TIME_LIB_H

#include <WiFiManager.h>
#include <TFT_eSPI.h>

class WifiTimeLib {
public:
    WifiTimeLib(const char* ntp_server, const char* tz_info);
    String getFormattedDate();
    String getFormattedTime();
    bool connectToWiFi(const char* ap_name, TFT_eSPI* tft);
    void configModeCallback(WiFiManager *wm, TFT_eSPI* tft);
    bool getNTPtime(int sec);
    void printNetworkInfo(TFT_eSPI* tft, String ssid = "", IPAddress ip = IPAddress());

private:
    tm timeinfo;
    time_t now;
    const char* NTP_SERVER;
    const char* TZ_INFO;
    WiFiManager wm;   // looking for credentials? don't need em! ... google "ESP32 WiFiManager"
};

#endif // WIFI_TIME_LIB_H
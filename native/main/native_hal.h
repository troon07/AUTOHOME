#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define INPUT_PULLDOWN 2
#define PROGMEM
static inline uint32_t millis(void) { return (uint32_t)(esp_timer_get_time()/1000); }
static inline uint32_t micros(void) { return (uint32_t)esp_timer_get_time(); }
static inline void delay(uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }
static inline void pinMode(int pin,int mode) {
    gpio_set_direction((gpio_num_t)pin,mode==OUTPUT?GPIO_MODE_OUTPUT:GPIO_MODE_INPUT);
    if(mode==INPUT_PULLDOWN) gpio_set_pull_mode((gpio_num_t)pin,GPIO_PULLDOWN_ONLY);
}
static inline void digitalWrite(int pin,int level) { gpio_set_level((gpio_num_t)pin,level); }
static inline int digitalRead(int pin) { return gpio_get_level((gpio_num_t)pin); }
void native_serial_begin(void);
#ifdef __cplusplus
#include <stdarg.h>
#include <string>
#include "esp_wifi.h"
#define WIFI_STA WIFI_MODE_STA
#define WIFI_AP_STA WIFI_MODE_APSTA
class NativeSerial {
public:
    void begin(unsigned) { native_serial_begin(); }
    int available(); int read();
    size_t print(const char *text);
    size_t printf(const char *format,...);
    explicit operator bool() const { return true; }
private:
    uint8_t received_[64]={}; size_t used_=0,index_=0;
};
class NativeESP {
public:
    uint32_t getFlashChipSize(); uint32_t getPsramSize();
};
bool psramFound();
extern NativeSerial Serial;
extern NativeESP ESP;
class IPAddress {
public:
    IPAddress(uint8_t a,uint8_t b,uint8_t c,uint8_t d):value_((uint32_t)a<<24|(uint32_t)b<<16|(uint32_t)c<<8|d) {}
    uint32_t value() const { return value_; }
    std::string toString() const;
private: uint32_t value_;
};
class NativeWiFi {
public:
    void mode(wifi_mode_t mode); void setSleep(bool enabled);
    bool softAP(const char *ssid,const char *password,int channel,bool hidden,int clients);
    IPAddress softAPIP() { return IPAddress(192,168,4,1); }
};
extern NativeWiFi WiFi;
class WiFiClient {
public:
    ~WiFiClient() { stop(); }
    void setTimeout(unsigned ms) { timeout_=ms; }
    bool connect(IPAddress ip,int port);
    size_t write(const uint8_t *data,size_t count);
    size_t print(const char *text) { return write((const uint8_t*)text,strlen(text)); }
    size_t printf(const char *format,...);
    bool connected(); int available(); int read(); void stop();
private: int socket_=-1,held_=-1; unsigned timeout_=1000;
};
#endif

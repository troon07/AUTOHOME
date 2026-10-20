#include "native_hal.h"
#include "hal/usb_serial_jtag_ll.h"
#include "esp_flash.h"
#include "esp32s3/spiram.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "lwip/sockets.h"
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>

NativeSerial Serial; NativeESP ESP; NativeWiFi WiFi;
static bool serial_ready=false,wifi_ready=false,wifi_started=false;
void native_serial_begin(void) {
    if(serial_ready) return;
    // Polling console avoids stale interrupt/ring-buffer state across Windows
    // USB reconnects. Only the low-priority diagnostics task uses these methods.
    usb_serial_jtag_ll_disable_intr_mask(USB_SERIAL_JTAG_LL_INTR_MASK);
    usb_serial_jtag_ll_clr_intsts_mask(USB_SERIAL_JTAG_LL_INTR_MASK);
    usb_serial_jtag_ll_txfifo_flush(); serial_ready=true;
}
int NativeSerial::available() {
    if(index_<used_) return used_-index_;
    if(serial_ready && usb_serial_jtag_ll_rxfifo_data_available()) {
        usb_serial_jtag_ll_clr_intsts_mask(USB_SERIAL_JTAG_INTR_SERIAL_OUT_RECV_PKT);
        used_=usb_serial_jtag_ll_read_rxfifo(received_,sizeof(received_)); index_=0; return used_;
    }
    return 0;
}
int NativeSerial::read() { return available()?received_[index_++]:-1; }
size_t NativeSerial::print(const char *text) {
    if(!serial_ready) return 0;
    size_t size=strlen(text),sent=0,last_packet=0; uint32_t start=millis(),progress=start;
    // SpeechTask can preempt this low-priority task for a model frame. A
    // 20 ms wall timeout truncated otherwise healthy JSON during inference.
    // Bound both stalled-host time and total scheduled transfer time.
    while(sent<size && (uint32_t)(millis()-start)<1000 && (uint32_t)(millis()-progress)<100) {
        if(usb_serial_jtag_ll_txfifo_writable()) {
            usb_serial_jtag_ll_clr_intsts_mask(USB_SERIAL_JTAG_INTR_SERIAL_IN_EMPTY);
            last_packet=usb_serial_jtag_ll_write_txfifo((const uint8_t*)text+sent,size-sent);
            sent+=last_packet;
            if(last_packet) progress=millis();
            usb_serial_jtag_ll_txfifo_flush();
        } else vTaskDelay(1);
    }
    // USB bulk transfers ending on a full 64-byte packet need a short packet
    // (here a ZLP). Otherwise Windows can hold a complete JSON record until
    // later console output, invalidating host-side fault-test timing.
    if(sent==size && last_packet==64) {
        while(!usb_serial_jtag_ll_txfifo_writable() && (uint32_t)(millis()-start)<1000 && (uint32_t)(millis()-progress)<100) vTaskDelay(1);
        if(usb_serial_jtag_ll_txfifo_writable()) usb_serial_jtag_ll_txfifo_flush();
    }
    return sent;
}
size_t NativeSerial::printf(const char *format,...) {
    // A fresh line lets host parsers recover from a truncated boot/USB message.
    char text[2048]; text[0]='\n'; va_list args; va_start(args,format); int n=vsnprintf(text+1,sizeof(text)-1,format,args); va_end(args);
    if(n<0 || n>=(int)sizeof(text)-1) return 0;
    return print(text);
}
uint32_t NativeESP::getFlashChipSize() { uint32_t bytes=0; esp_flash_get_size(nullptr,&bytes); return bytes; }
uint32_t NativeESP::getPsramSize() { return psramFound()?esp_spiram_get_size():0; }
bool psramFound() { return esp_spiram_is_initialized(); }
std::string IPAddress::toString() const {
    char text[20]; snprintf(text,sizeof(text),"%u.%u.%u.%u",(unsigned)(value_>>24),(unsigned)(value_>>16)&255,(unsigned)(value_>>8)&255,(unsigned)value_&255); return text;
}
void NativeWiFi::mode(wifi_mode_t mode) {
    if(!wifi_ready) {
        ESP_ERROR_CHECK(esp_netif_init()); esp_err_t e=esp_event_loop_create_default();
        if(e!=ESP_OK && e!=ESP_ERR_INVALID_STATE) ESP_ERROR_CHECK(e);
        esp_netif_create_default_wifi_sta(); esp_netif_create_default_wifi_ap();
        wifi_init_config_t config=WIFI_INIT_CONFIG_DEFAULT(); ESP_ERROR_CHECK(esp_wifi_init(&config));
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM)); wifi_ready=true;
    }
    ESP_ERROR_CHECK(esp_wifi_set_mode(mode));
    if(!wifi_started) { ESP_ERROR_CHECK(esp_wifi_start()); wifi_started=true; }
}
void NativeWiFi::setSleep(bool enabled) { ESP_ERROR_CHECK(esp_wifi_set_ps(enabled?WIFI_PS_MIN_MODEM:WIFI_PS_NONE)); }
bool NativeWiFi::softAP(const char *ssid,const char *password,int channel,bool hidden,int clients) {
    if(strlen(ssid)>32 || !password || strlen(password)<8 || strlen(password)>63) return false;
    wifi_config_t config={}; memcpy(config.ap.ssid,ssid,strlen(ssid)); config.ap.ssid_len=strlen(ssid);
    memcpy(config.ap.password,password,strlen(password)); config.ap.authmode=WIFI_AUTH_WPA2_PSK;
    config.ap.channel=channel; config.ap.ssid_hidden=hidden; config.ap.max_connection=clients;
    return esp_wifi_set_config(WIFI_IF_AP,&config)==ESP_OK;
}
bool WiFiClient::connect(IPAddress ip,int port) {
    socket_=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP); if(socket_<0) return false;
    timeval time={int(timeout_/1000),int(timeout_%1000)*1000};
    setsockopt(socket_,SOL_SOCKET,SO_RCVTIMEO,&time,sizeof(time)); setsockopt(socket_,SOL_SOCKET,SO_SNDTIMEO,&time,sizeof(time));
    sockaddr_in address={}; address.sin_family=AF_INET; address.sin_port=htons(port); address.sin_addr.s_addr=htonl(ip.value());
    if(::connect(socket_,(sockaddr*)&address,sizeof(address))) { stop(); return false; } return true;
}
size_t WiFiClient::write(const uint8_t *data,size_t count) {
    size_t done=0; while(socket_>=0 && done<count) { int n=send(socket_,data+done,count-done,0); if(n<=0) break; done+=n; } return done;
}
size_t WiFiClient::printf(const char *format,...) {
    char text[512]; va_list args; va_start(args,format); int n=vsnprintf(text,sizeof(text),format,args); va_end(args);
    return n>0 && n<(int)sizeof(text)?write((const uint8_t*)text,n):0;
}
int WiFiClient::available() {
    if(held_>=0) return 1;
    uint8_t byte;
    if(socket_>=0 && recv(socket_,&byte,1,MSG_DONTWAIT)==1) { held_=byte; return 1; }
    return 0;
}
bool WiFiClient::connected() {
    if(held_>=0) return true;
    if(socket_<0) return false;
    char byte; int n=recv(socket_,&byte,1,MSG_PEEK|MSG_DONTWAIT);
    return n>0 || (n<0 && (errno==EWOULDBLOCK || errno==EAGAIN));
}
int WiFiClient::read() { if(!available()) return -1; int result=held_; held_=-1; return result; }
void WiFiClient::stop() { if(socket_>=0) { close(socket_); socket_=-1; } held_=-1; }

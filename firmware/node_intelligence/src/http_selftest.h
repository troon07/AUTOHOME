#pragma once
#include "network_hal.h"
static void (*s_http_test_service)()=nullptr;

inline bool http_check(const char *method,const char *path,const char *body,const char *token,
    int expected,const char *needle=nullptr) {
    WiFiClient client; client.setTimeout(1000);
    if(!client.connect(IPAddress(127,0,0,1),80)) return false;
    size_t len=body?strlen(body):0;
    client.printf("%s %s HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\nContent-Type: text/plain\r\nContent-Length: %u\r\n",method,path,(unsigned)len);
    if(token) client.printf("X-ClawGrid-Token: %s\r\n",token);
    client.print("\r\n"); if(len) client.write((const uint8_t*)body,len);
    char response[2048]; size_t used=0,total_received=0; uint32_t start=millis();
    while((client.connected()||client.available()) && (uint32_t)(millis()-start)<1500) {
        while(client.available()) { char c=client.read(); ++total_received; if(used+1<sizeof(response)) response[used++]=c; }
        response[used]=0;
        char *header_end=strstr(response,"\r\n\r\n"),*content=strstr(response,"Content-Length:");
        unsigned body_size=0;
        if(header_end && content && content<header_end && sscanf(content,"Content-Length: %u",&body_size)==1 &&
           total_received>=(size_t)(header_end+4-response)+body_size) break;
        if(s_http_test_service) s_http_test_service();
        delay(1);
    }
    client.stop(); response[used]=0; int code=0;
    return sscanf(response,"HTTP/1.1 %d",&code)==1 && code==expected && (!needle || strstr(response,needle));
}
inline void http_selftest(void (*service)()) {
    s_http_test_service=service;
    int passed=0,total=0;
    auto check=[&](const char *name,bool result) {
        ++total; if(result) ++passed;
        Serial.printf("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"http_case\",\"case\":\"%s\",\"passed\":%s}\n",name,result?"true":"false");
    };
    check("state",http_check("GET","/api/state",nullptr,nullptr,200,"bedroom"));
    check("memory",http_check("GET","/api/memory",nullptr,nullptr,200,"HARDWARE_MEASURED"));
    check("rtos_telemetry",http_check("GET","/api/rtos",nullptr,nullptr,200,"\"watchdog\":true"));
    check("simulation_page",http_check("GET","/simulation",nullptr,nullptr,200,"house laboratory"));
    check("simulation_source_label",http_check("GET","/api/simulation",nullptr,nullptr,200,"\"input_source\":\"SIMULATED\""));
    check("unauthorized_simulation",http_check("POST","/api/simulation/input","58 300 80 56 300 80 56 300 80 56 300 80",nullptr,401));
    check("malformed_simulation",http_check("POST","/api/simulation/input","58 300 80",kApiToken,400));
    check("invalid_simulation_flags",http_check("POST","/api/simulation/input","64 300 80 56 300 80 56 300 80 56 300 80",kApiToken,400));
    check("simulation_input_queue",http_check("POST","/api/simulation/input","58 300 80 56 300 80 56 300 80 56 300 80",kApiToken,202,"\"queued\":true"));
    delay(400);
    check("simulation_board_occupancy",http_check("GET","/api/simulation",nullptr,nullptr,200,"\"occupied\":1"));
    check("simulation_board_outputs",http_check("GET","/api/simulation",nullptr,nullptr,200,"\"outputs\":3"));
    delay(3200);
    check("simulation_input_expiry",http_check("GET","/api/simulation",nullptr,nullptr,200,"\"sensor_known\":0"));
    check("unauthorized_control",http_check("POST","/api/control","1 0 0 0",nullptr,401));
    check("wrong_token",http_check("POST","/api/control","1 0 0 0","invalid",401));
    check("unauthorized_transaction",http_check("GET","/api/transaction",nullptr,nullptr,401));
    check("authorized_transaction",http_check("GET","/api/transaction",nullptr,kApiToken,200,"outcome_known"));
    check("parser_preview",http_check("POST","/api/voice","turn on bedroom light",nullptr,200,"\"authorized\":false"));
    check("parser_ambiguity",http_check("POST","/api/voice","turn on light",nullptr,200,"\"requires_clarification\":true"));
    check("bad_control",http_check("POST","/api/control","1 999 1 0",kApiToken,400));
    check("overflow_control",http_check("POST","/api/control","1 0 1 4294967296",kApiToken,400));
    char oversized[256]; memset(oversized,'a',sizeof(oversized)-1); oversized[255]=0;
    check("oversized_body",http_check("POST","/api/voice",oversized,nullptr,400));
    check("unknown_route",http_check("GET","/missing",nullptr,nullptr,404));
    check("authorized_control_queue",http_check("POST","/api/control","1 0 0 0",kApiToken,202,"\"queued\":true"));
    Serial.printf("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"http_selftest\",\"passed\":%s,\"passed_cases\":%d,\"total_cases\":%d}\n",passed==total?"true":"false",passed,total);
    s_http_test_service=nullptr;
}

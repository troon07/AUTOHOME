#pragma once
#include "network_hal.h"

// USB transport to the same SDK HTTP handlers used by the AP browser. The
// desktop never computes occupancy, model inference or actuator decisions.
inline void serial_http_bridge(const char *header) {
    unsigned id=0,length=0; char method=0,path[48]={},token[64]={},extra=0;
    if(sscanf(header,"@%u %c %47s %u %63s %c",&id,&method,path,&length,token,&extra)!=5 ||
       !id || (method!='G' && method!='P') || strncmp(path,"/api/",5) || length>128000 ||
       (method=='G' && length)) return;
    for(const char *p=path;*p;++p) if(!((*p>='a'&&*p<='z')||*p=='/'||*p=='_')) return;
    WiFiClient client; client.setTimeout(3000);
    if(!client.connect(IPAddress(127,0,0,1),80)) {
        Serial.printf("{\"kind\":\"bridge\",\"id\":%u,\"status\":502,\"body\":{\"error\":\"ESP32 HTTP unavailable\"}}\n",id); return;
    }
    client.printf("%s %s HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\nContent-Type: application/octet-stream\r\nContent-Length: %u\r\n",method=='G'?"GET":"POST",path,length);
    if(strcmp(token,"-")) client.printf("X-ClawGrid-Token: %s\r\n",token);
    client.print("\r\n");
    uint32_t started=millis(); unsigned received=0; uint8_t block[256];
    while(received<length && (uint32_t)(millis()-started)<6000) {
        unsigned count=0;
        while(count<sizeof(block) && received+count<length && Serial.available()) block[count++]=Serial.read();
        if(count) { if(client.write(block,count)!=count) break; received+=count; }
        else delay(1);
    }
    // A truncated upload is rejected; no browser request is replayed here.
    static char response[4096]; size_t used=0;
    while(received==length && (client.connected()||client.available()) && (uint32_t)(millis()-started)<10000) {
        while(client.available()) { int c=client.read(); if(c>=0 && used+1<sizeof(response)) response[used++]=(char)c; }
        response[used]=0;
        char *header_end=strstr(response,"\r\n\r\n"),*content=strstr(response,"Content-Length:");
        unsigned body_size=0;
        if(header_end && content && content<header_end && sscanf(content,"Content-Length: %u",&body_size)==1 &&
           body_size<=1536 && used>=(size_t)(header_end+4-response)+body_size) {
            used=(header_end+4-response)+body_size; break;
        }
        delay(1);
    }
    client.stop(); response[used]=0; int status=0;
    char *body=strstr(response,"\r\n\r\n");
    if(received!=length || !body || sscanf(response,"HTTP/1.1 %d",&status)!=1 || strlen(body+4)>1536 || body[4]!='{') {
        Serial.printf("{\"kind\":\"bridge\",\"id\":%u,\"status\":502,\"body\":{\"error\":\"Incomplete board response\",\"received\":%u,\"response_bytes\":%u}}\n",id,received,(unsigned)used); return;
    }
    Serial.printf("{\"kind\":\"bridge\",\"id\":%u,\"status\":%d,\"body\":%s}\n",id,status,body+4);
}

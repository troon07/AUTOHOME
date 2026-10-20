#pragma once
#include "platform_hal.h"
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <esp_task_wdt.h>
#include <atomic>
#include <stdarg.h>

// Project task memory is allocated at compile time in internal SRAM.
template<class T,size_t Depth> class StaticMailbox {
public:
    void begin() { queue_=xQueueCreateStatic(Depth,sizeof(T),storage_,&control_); }
    bool push(const T &value) { return queue_ && xQueueSend(queue_,&value,0)==pdTRUE; }
    bool pop(T &value) { return queue_ && xQueueReceive(queue_,&value,0)==pdTRUE; }
    void latest(const T &value) { static_assert(Depth==1,"Overwrite requires depth one"); if(queue_) xQueueOverwrite(queue_,&value); }
private:
    QueueHandle_t queue_=nullptr; StaticQueue_t control_={};
    alignas(4) uint8_t storage_[Depth*sizeof(T)]={};
};

struct LogLine { char text[384]; };
class DiagnosticLog {
public:
    void begin() { queue_.begin(); }
    void write(const char *format,...) {
        LogLine line; va_list args; va_start(args,format);
        int n=vsnprintf(line.text,sizeof(line.text),format,args); va_end(args);
        if(n<0 || n>=(int)sizeof(line.text) || !queue_.push(line)) ++dropped;
    }
    void drain() { LogLine line; for(int i=0;i<8&&queue_.pop(line);++i) if(Serial) Serial.print(line.text); }
    std::atomic<uint32_t> dropped{0};
private:
    StaticMailbox<LogLine,16> queue_;
};

struct TaskTiming {
    std::atomic<uint32_t> jobs{0},max_exec_us{0},max_late_us{0},deadline_misses{0};
    std::atomic<bool> watchdog{false};
    TaskHandle_t handle=nullptr;
    static void maximum(std::atomic<uint32_t> &out,uint32_t value) {
        uint32_t old=out.load(); while(old<value && !out.compare_exchange_weak(old,value)) {}
    }
    void record(uint32_t start,uint32_t expected,uint32_t period_us) {
        uint32_t elapsed=micros()-start; int32_t signed_late=(int32_t)(start-expected);
        uint32_t late=signed_late>0?(uint32_t)signed_late:0;
        maximum(max_exec_us,elapsed); maximum(max_late_us,late); ++jobs;
        if((uint64_t)elapsed+late>period_us) ++deadline_misses;
    }
    void log(const char *node,const char *task,int core,int priority,int period_ms,uint32_t diagnostic_drops) {
        if(!Serial || !handle) return;
        Serial.printf("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"rtos\",\"node\":\"%s\",\"task\":\"%s\",\"core\":%d,\"priority\":%d,\"period_ms\":%d,\"jobs\":%u,\"max_exec_us\":%u,\"max_late_us\":%u,\"deadline_misses\":%u,\"stack_free_bytes\":%u,\"watchdog\":%s,\"diagnostic_dropped\":%u}\n",node,task,core,priority,period_ms,jobs.load(),max_exec_us.load(),max_late_us.load(),deadline_misses.load(),(unsigned)uxTaskGetStackHighWaterMark(handle),watchdog.load()?"true":"false",diagnostic_drops);
    }
};

static constexpr EventBits_t kControlReady=1,kLinkReady=2,kTaskFault=4;
class RuntimeHealth {
public:
    bool begin() { group_=xEventGroupCreateStatic(&storage_); return group_ && esp_task_wdt_init(5,true)==ESP_OK; }
    void ready(EventBits_t bit,TaskTiming &timing) {
        bool ok=esp_task_wdt_add(nullptr)==ESP_OK; timing.watchdog.store(ok);
        xEventGroupSetBits(group_,ok?bit:kTaskFault);
    }
    bool healthy(EventBits_t required) { if(!group_) return false; EventBits_t bits=xEventGroupGetBits(group_); return (bits&required)==required && !(bits&kTaskFault); }
    void fail() { if(group_) xEventGroupSetBits(group_,kTaskFault); }
private:
    StaticEventGroup_t storage_={}; EventGroupHandle_t group_=nullptr;
};

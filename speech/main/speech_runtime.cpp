#include "speech_runtime.h"
#include "native_hal.h"
#include "esp_heap_caps.h"
#include "esp_mn_models.h"
#include "esp_mn_speech_commands.h"
#include "model_path.h"
#include <algorithm>
#include <ctype.h>

static portMUX_TYPE lock=portMUX_INITIALIZER_UNLOCKED;
static SpeechSnapshot state;
static TaskHandle_t worker=nullptr; static StaticTask_t tcb; static StackType_t stack[12288];
static uint8_t *clip=nullptr; static size_t clip_bytes=0;
static const esp_mn_iface_t *engine=nullptr; static model_iface_data_t *model=nullptr;
static srmodel_list_t *models=nullptr;
static constexpr size_t kMaxClipBytes=128000;

static bool initialize() {
    // Keep medium-sized libc allocations in PSRAM. DMA and task stacks keep
    // their explicit internal-memory capabilities in ESP-IDF.
    heap_caps_malloc_extmem_enable(1024);
    if(heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)<80000 ||
       heap_caps_get_free_size(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT)<5*1024*1024) return false;
    clip=(uint8_t*)heap_caps_aligned_alloc(16,kMaxClipBytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(!clip) return false;
    models=esp_srmodel_init("model"); if(!models) return false;
    char *name=esp_srmodel_filter(models,ESP_MN_PREFIX,ESP_MN_ENGLISH); if(!name) return false;
    engine=esp_mn_handle_from_name(name); if(!engine) return false;
    model=engine->create(name,6000); if(!model || engine->get_samp_rate(model)!=16000) return false;
    if(esp_mn_commands_alloc(engine,model)!=ESP_OK || esp_mn_commands_clear()!=ESP_OK) return false;
    const char *rooms[]={"bedroom","living room","kitchen","study"};
    char phrase[64];
    for(int device=0;device<8;++device) for(int action=0;action<2;++action) {
        snprintf(phrase,sizeof(phrase),"turn %s %s %s",action?"off":"on",rooms[device/2],device%2?"fan":"light");
        for(char *p=phrase;*p;++p) *p=(char)toupper((unsigned char)*p);
        if(esp_mn_commands_add(device*2+action+1,phrase)!=ESP_OK) return false;
    }
    if(esp_mn_commands_update()!=nullptr) return false;
    engine->set_det_threshold(model,0.7f);
    return engine->get_samp_chunksize(model)>0 && engine->get_samp_chunksize(model)<=1024;
}
static void run(void*) {
    bool ready=initialize();
    portENTER_CRITICAL(&lock); state.phase=ready?1:5; portEXIT_CRITICAL(&lock);
    if(!ready) {
        esp_mn_commands_free();
        if(model) engine->destroy(model);
        if(models) esp_srmodel_deinit(models);
        if(clip) heap_caps_free(clip);
        clip=nullptr; vTaskDelete(nullptr); return;
    }
    alignas(16) int16_t frame[1024]; int chunk=engine->get_samp_chunksize(model);
    for(;;) {
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        SpeechSnapshot result; size_t bytes;
        portENTER_CRITICAL(&lock); result=state; bytes=clip_bytes; portEXIT_CRITICAL(&lock);
        engine->clean(model); uint32_t start=micros();
        result.frames=0; result.max_frame_us=0; result.command_id=0; result.confidence=0;
        // Include bounded trailing silence to finalize a command. The browser
        // only captures PCM; all model inference runs in this ESP32 task.
        for(size_t offset=0;offset<bytes/2+16000;offset+=chunk) {
            memset(frame,0,chunk*sizeof(int16_t));
            if(offset<bytes/2) memcpy(frame,clip+offset*2,std::min((size_t)chunk,bytes/2-offset)*2);
            uint32_t frame_start=micros(); esp_mn_state_t detected=engine->detect(model,frame);
            result.max_frame_us=std::max(result.max_frame_us,micros()-frame_start); ++result.frames;
            if(detected==ESP_MN_STATE_DETECTED) {
                esp_mn_results_t *found=engine->get_results(model);
                if(found && found->num>0 && found->command_id[0]>=1 && found->command_id[0]<=16 && found->prob[0]>=0.7f) {
                    result.command_id=found->command_id[0]; result.confidence=found->prob[0]; break;
                }
            }
            if(detected==ESP_MN_STATE_TIMEOUT) break;
            vTaskDelay(1); // Yield to LinkTask and HTTP; no GPIO ownership here.
        }
        result.inference_us=micros()-start; result.phase=4; result.samples=bytes/2;
        // Diagnostic only: bounded ASCII decoder text, never authorization.
        // Safe alphabet avoids injecting JSON delimiters into the API.
        esp_mn_results_t *final_result=engine->get_results(model);
        memset(result.diagnostic_text,0,sizeof(result.diagnostic_text));
        if(final_result) for(size_t i=0;i+1<sizeof(result.diagnostic_text) && final_result->string[i];++i) {
            char c=final_result->string[i];
            result.diagnostic_text[i]=((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c==' ')?c:'_';
        }
        result.stack_free=uxTaskGetStackHighWaterMark(nullptr);
        memset(clip,0,bytes); // Do not retain microphone content after processing.
        portENTER_CRITICAL(&lock); state=result; portEXIT_CRITICAL(&lock);
    }
}
bool speech_begin() {
    worker=xTaskCreateStaticPinnedToCore(run,"SpeechTask",sizeof(stack),nullptr,2,stack,&tcb,1);
    return worker!=nullptr;
}
uint8_t *speech_reserve_clip(size_t bytes,uint32_t &job) {
    if(bytes<3200 || bytes>kMaxClipBytes || (bytes&1)) return nullptr;
    bool accepted=false;
    portENTER_CRITICAL(&lock);
    if((state.phase==1 || state.phase==4) && clip) {
        state.phase=2; ++state.job; state.command_id=0; state.confidence=0; clip_bytes=bytes; job=state.job; accepted=true;
    }
    portEXIT_CRITICAL(&lock); return accepted?clip:nullptr;
}
void speech_finish_clip(bool complete) {
    uint32_t crc=0xffffffffu;
    if(complete) for(size_t i=0;i<clip_bytes;++i) {
        crc^=clip[i];
        for(int bit=0;bit<8;++bit) crc=(crc>>1)^(0xedb88320u & (0u-(crc&1u)));
    }
    portENTER_CRITICAL(&lock); state.pcm_crc32=complete?~crc:0; state.phase=complete?3:1; portEXIT_CRITICAL(&lock);
    if(complete) xTaskNotifyGive(worker);
    else if(clip) memset(clip,0,clip_bytes);
}
SpeechSnapshot speech_snapshot() {
    portENTER_CRITICAL(&lock); SpeechSnapshot copy=state; portEXIT_CRITICAL(&lock); return copy;
}

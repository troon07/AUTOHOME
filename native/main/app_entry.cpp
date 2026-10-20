// Native ESP-IDF entry point: no Arduino core or Arduino runtime is linked.
#include "native_hal.h"
#include "nvs_flash.h"
#include "esp_ota_ops.h"
#if defined(CLAWGRID_NODE_A)
#include "../../firmware/node_voice/src/main.cpp"
#elif defined(CLAWGRID_NODE_B)
#include "../../firmware/node_intelligence/src/main.cpp"
#else
#error Select the board role in the build configuration
#endif

static StaticTask_t diagnostic_tcb;
static StackType_t diagnostic_stack[8192];
static void diagnostics(void*) { for(;;) loop(); }
extern "C" void app_main(void) {
    // Preserve NVS on errors: erasing it would destroy replay/policy history.
    esp_err_t status=nvs_flash_init();
    if(status!=ESP_OK) { native_serial_begin(); printf("NVS initialization failed: %s\n",esp_err_to_name(status)); return; }
    setup();
    xTaskCreateStaticPinnedToCore(diagnostics,"Diagnostics",sizeof(diagnostic_stack),nullptr,1,diagnostic_stack,&diagnostic_tcb,1);
    // Only confirm a staged image once its essential tasks have registered.
    const esp_partition_t *running=esp_ota_get_running_partition();
    esp_ota_img_states_t image_state;
    if(running && esp_ota_get_state_partition(running,&image_state)==ESP_OK && image_state==ESP_OTA_IMG_PENDING_VERIFY) {
        EventBits_t required=kLinkReady;
#if defined(CLAWGRID_NODE_A)
        required|=kControlReady;
#endif
        for(int i=0;i<200 && !s_health.healthy(required);++i) vTaskDelay(pdMS_TO_TICKS(10));
        if(s_ok && s_health.healthy(required)) ESP_ERROR_CHECK(esp_ota_mark_app_valid_cancel_rollback());
        else esp_ota_mark_app_invalid_rollback_and_reboot();
    }
}

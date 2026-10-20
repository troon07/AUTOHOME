#include "safety_selftest.h"
#include "safety_kernel.h"
#include <stddef.h>

bool safety_boot_selftest(void) {
    bool passed = true;
    const char *reason = NULL;
    safety_kernel_init();
    passed &= safety_kernel_register_device("bedroom","light",DEVICE_TYPE_MOCK,-1,false)==0;
    passed &= safety_kernel_register_device("bedroom","light",DEVICE_TYPE_MOCK,-1,false)==-1;
    passed &= safety_kernel_register_device("room_name_too_long","fan",DEVICE_TYPE_MOCK,-1,false)==-1;
    passed &= !safety_kernel_request_action("bedroom","light",(action_type_t)99,false,0,&reason);
    passed &= safety_kernel_request_action("bedroom","light",ACTION_ON,false,0,&reason);
    passed &= !safety_kernel_request_action("bedroom","light",ACTION_OFF,false,1,&reason);
    safety_kernel_tick_periodic(DEFAULT_MAX_RUN_MS);
    passed &= !safety_kernel_get_device(0)->is_on;
    safety_kernel_init();
    safety_kernel_register_device("bedroom","light",DEVICE_TYPE_MOCK,-1,false);
    passed &= safety_kernel_request_action("bedroom","light",ACTION_ON,false,0xfffffff0u,&reason);
    passed &= !safety_kernel_request_action("bedroom","light",ACTION_OFF,false,0x10u,&reason);
    passed &= safety_kernel_request_action("bedroom","light",ACTION_OFF,false,0x1000u,&reason);
    safety_kernel_init();
    return passed;
}

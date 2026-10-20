#pragma once
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Boot only, before registering real GPIO: temporarily resets the device table. */
bool safety_boot_selftest(void);
#ifdef __cplusplus
}
#endif

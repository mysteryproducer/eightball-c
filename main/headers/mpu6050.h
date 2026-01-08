#pragma once
#include <stdint.h>
#if __cplusplus
extern "C" {
#endif
    void wake_loop(void (*shakeCB)(void), void (*idleCB)(void), void (*otherCB)(uint8_t));
#if __cplusplus
}
#endif
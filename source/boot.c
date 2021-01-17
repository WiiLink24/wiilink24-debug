// Derived from source/usbloader/disc.c.
#include <ogcsys.h>

#include "channel_load.h"
#include "helpers.h"
#include "memory_layout.h"

void *channel_entrypoint = 0;
// From channel_load, taken from TMD
extern u8 channelios;

void jump_to_entrypoint(void *local_entrypoint) {
    // Change IOS.
    __IOS_LaunchNewIOS(channelios);

    // Set up existing memory
    set_low_mem(channelios);
    set_temporary_time();

    // Shutdown IOS subsystems
    extern void __exception_closeall();
    u32 level = IRQ_Disable();
    __IOS_ShutdownSubsystems();
    __exception_closeall();

    // Ensure we have a valid entrypoint.
    if ((u32)local_entrypoint < 0x80000000) {
        local_entrypoint += 0x80000000;
    }

    channel_entrypoint = local_entrypoint;

    // We don't need program arguments.
    *(vu32 *)0xCC003024 = 1;

    // if (shouldHook) {
    //     asm volatile("lis %r3, channel_entrypoint@h\n"
    //                  "ori %r3, %r3, channel_entrypoint@l\n"
    //                  "lwz %r3, 0(%r3)\n"
    //                  "mtlr %r3\n"
    //                  "lis %r3, 0x8000\n"
    //                  "ori %r3, %r3, 0x18A8\n"
    //                  "nop\n"
    //                  "mtctr %r3\n"
    //                  "bctr\n");
    // } else {
    asm volatile("lis %r3, channel_entrypoint@h\n"
                 "ori %r3, %r3, channel_entrypoint@l\n"
                 "lwz %r3, 0(%r3)\n"
                 "mtlr %r3\n"
                 "blr\n");
    // }

    IRQ_Restore(level);

    // There is no return.
    // We are done.
    // Until next time, we meet again...
    return;
}

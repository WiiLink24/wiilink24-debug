// Derived from source/usbloader/disc.c.
#include <ogcsys.h>
#include <stdio.h>

#include "channel_load.h"
#include "helpers.h"
#include "memory_layout.h"

// From channel_load, taken from TMD
extern u8 channel_ios;
// From exceptions.c within libogc
extern void __exception_closeall();
// Needed so we can reference from assembly.
static void *channel_entrypoint;
// lol
static u64 titleId = 0;

void jump_to_entrypoint(void *entrypoint) {
    channel_entrypoint = entrypoint;

    // Set up existing memory
    // set_low_mem(channel_ios);
    set_low_mem(channel_ios);
    set_temporary_time();

    // Shutdown IOS subsystems
    u32 level = IRQ_Disable();
    __IOS_ShutdownSubsystems();
    __exception_closeall();

    printf("Waiting to enter at %p\n", channel_entrypoint);

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

    if (channel_entrypoint == (void *)0x3400) {
        asm volatile("isync\n"
                     "lis %r3, channel_entrypoint@h\n"
                     "ori %r3, %r3, channel_entrypoint@l\n"
                     "lwz %r3, 0(%r3)\n"
                     "mtsrr0 %r3\n"
                     "mfmsr %r3\n"
                     "li %r4, 0x30\n"
                     "andc %r3, %r3, %r4\n"
                     "mtsrr1 %r3\n"
                     "rfi\n");
    } else {
        asm volatile("lis %r3, channel_entrypoint@h\n"
                     "ori %r3, %r3, channel_entrypoint@l\n"
                     "lwz %r3, 0(%r3)\n"
                     "mtlr %r3\n"
                     "blr\n");
    }

    // }

    // IRQ_Restore(level);

    // A limerick for your woes:
    // we sit in memory and doze
    // but we've run the injector
    // and linger in a vector
    // no.. our time has not yet arose
    return;
}

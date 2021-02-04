// Derived from source/usbloader/disc.c.
#include <libpatcher/libpatcher.h>
#include <ogc/machine/processor.h>
#include <ogcsys.h>
#include <stdio.h>
#include <string.h>

#include "boot.h"
#include "channel_load.h"
#include "helpers.h"
#include "loader_stub_bin.h"
#include "memory_layout.h"

#define LOADER_MEMORY_START (u32 *)0x81330000
#define LOADER_MEMORY_END (u32 *)0x81800000

// From channel_load, taken from TMD
extern u8 channel_ios;
// From exceptions.c within libogc
extern void __exception_closeall();
// Needed so we can reference from assembly.
static void *channel_entrypoint;
// Necessary as to allow our stub to run.
static void *vector_area;

void jump_to_entrypoint(void *entrypoint) {
    channel_entrypoint = entrypoint;

    // Our stub entrypoint can be starting at the base of the vector area.
    vector_area = (void *)Vector_Area;

    // Set up existing memory
    set_low_mem(channel_ios);
    set_temporary_time();

    // Shutdown IOS subsystems
    u32 level = IRQ_Disable();
    __IOS_ShutdownSubsystems();
    __exception_closeall();

    printf("Waiting to enter at %p\n", channel_entrypoint);

    // Write reset code
    write32(Reset_Code, 1);

    // Copy our stub.
    memcpy(vector_area, loader_stub_bin, loader_stub_bin_size);

    // Patch the game so that the NAND Boot Program launches our
    // stub instead of continuing on to normal activities.
    bool nand_patched = patch_memory_range(LOADER_MEMORY_START,
                                           LOADER_MEMORY_END, nand_loader_old,
                                           nand_loader_patch, NAND_LOADER_SIZE);
    if (!nand_patched) {
        printf("unable to find and patch the NAND Boot Program!\n");
        return;
    }

    // channel_entrypoint is most likely 0x3400, as we are dealing with.
    if (channel_entrypoint == (void *)0x3400) {
        asm volatile("lis %r3, channel_entrypoint@h\n"
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

    IRQ_Restore(level);

    // A limerick for your woes:
    // we sit in memory and doze
    // but we've run the injector
    // and linger in a vector
    // no.. our time has not yet arose
    return;
}

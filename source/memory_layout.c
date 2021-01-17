// Derived from source/memory/memory.h within usbloadergx and source/disc.c
// within wiilauncher.
#include <ogc/lwp_watchdog.h>
#include <string.h>

#include "helpers.h"
#include "memory_layout.h"

void prepare_low_mem() {
    *BI2 = 0x817E5480;
    // 32Mhz on Bus
    *(vu32 *)0xCD00643C = 0x00000000;

    memset((u8 *)Disc_ID, 0, 32);
    memset((void *)0x80001800, 0, 0x1800);
    DCFlushRange((void *)0x80000000, 0x3f00);
}

void set_low_mem(u32 ios_version) {
    // Standard Boot Code
    *Sys_Magic = 0x0D15EA5E;
    // Version
    *Sys_Version = 0x00000001;
    // Arena Low
    *Arena_L = 0x00000000;
    // Console Bus Speed
    *Bus_Speed = 0x0E7BE2C0;
    // Console CPU Speed
    *CPU_Speed = 0x2B73A840;
    // Assembler
    *Assembler = 0x38A00040;
    // Thread Init
    *OS_Thread = 0x80431A80;
    // Dev Debugger Monitor Address
    *Dev_Debugger = 0x81800000;
    // Simulated Memory Size
    *Simulated_Mem = 0x01800000;
    // Fix for Sam & Max (WiiPower)
    *GameID_Address = 0x80000000;

    memcpy((void *)Online_Check, (void *)Disc_ID, 4);
    memcpy((void *)Vector_Area, (void *)Disc_ID, 8);
    *Current_IOS = (ios_version << 16) | 0xffff;
    *Apploader_IOS = (ios_version << 16) | 0xffff;

    DCFlushRange((void *)0x80000000, 0x3f00);
}

void set_temporary_time() { settime(secs_to_ticks(time(NULL) - 946684800)); }

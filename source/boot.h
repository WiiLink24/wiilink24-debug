#include <ogcsys.h>

static const u16 nand_loader_old[] = {
    0x8001, 0x0034, // lwz r0,0x34(r1) ; load from stack
    0x83e1, 0x002c, // lwz r31,0x2c(r1)
    0x83c1, 0x0028, // lwz r30,0x28(r1)
    0x83a1, 0x0024, // lwz r29,0x24(r1)
    0x8381, 0x0020, // lwz r28,0x20(r1)
    0x7c08, 0x03a6, // mtspr LR,r0     ; set LR to data from r0
    0x3821, 0x0030, // addi r1,r1,0x30 ; wipe stack possibly?
    0x4e80, 0x0020  // blr ; launch!
};
static const u16 nand_loader_patch[] = {
    0x3c00, 0x8000, // lis r0, 0x8000
    0x6000, 0x1800, // ori r0, r0, 0x1800 ; load 0x80001800 to r0
    0x7c08, 0x03a6, // mtspr LR,r0     ; set LR to data from r0
    0x4e80, 0x0020, // blr
    0x6000, 0x0000, // nop
    0x6000, 0x0000, // nop
    0x6000, 0x0000, // nop
    0x6000, 0x0000  // nop
};

#define NAND_LOADER_SIZE sizeof(nand_loader_patch)

void jump_to_entrypoint(void *channel_entrypoint);

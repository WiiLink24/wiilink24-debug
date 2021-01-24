# TODO: https://sourceware.org/binutils/docs/as/index.html#SEC_Contents
# TODO: Hook 802a95d0 or reimpl for append usage

# .data will be at the start of our loading area.
# We will use it to store pointers (updated by our corresponding Homebrew app).
# Additionally, we can retain state.
.data
printf_ptr:
    .int 0x802a4fe8
EverythingIsInitialized:
    .byte 0
msg:
    .string "This is a test.\n"
    len = . - msg       # length of our dear string


# Our code will reside in .text.
# We need to align it to 8 to allow execution.
.text
.balign 8
_entrypoint:
    # Currently the same as the offical loader to verify stub operation.
    lwz r0,0x34(r1)  # load loaded game address from stack
    lwz r31,0x2c(r1)
    lwz r30,0x28(r1)
    lwz r29,0x24(r1)
    lwz r28,0x20(r1)
    mtspr LR,r0      # set LR to data from r0
    addi r1,r1,0x30
    blr              # launch!

    .global OSReportHook
OSReportHook:
    # Load the address of printf to r2.
    lis r2, printf_ptr@h
    ori r2, r2, printf_ptr@l
    # Move our own message in.
    lis r3, msg@h
    ori r3, r3, msg@l
    # Branch to our address loaded in r2, print.
    mtctr r2
    bctr
    # lis  r4, EverythingIsInitialized@h
    # ori  r4, r4, EverythingIsInitialized@l
    # lwz r3, 0(r4)
    # cmpwi r3, 0x1
    # bne [Initialize]

Initialize:
    nop

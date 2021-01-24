/* We need this file in order to properly lay out where exactly the unused
   Exception Vector area is.
   See also: https://wiibrew.org/wiki/Memory_Map
*/

MEMORY {
    vector(rwx) : ORIGIN = 0x80001800, LENGTH = 0x1800
}

SECTIONS {
    /* Our actual code.
    We expect 0x80000000 to be our immediate entrypoint. */
    .text : {
        *(.text)
    } >vector

    /* Informational data we need to operate. */
    .data : {
        _DataStart = . ;
        *(.data)
        _DataEnd   = . ;
    } >vector
}

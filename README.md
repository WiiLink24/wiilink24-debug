# wiilink24-debug
`wiilink24-debug` is a homebrew utility dedicated to debugging Wii channels.

With the help of a USB Gecko or a Shuriken, one can redirect OSReport and other related
logging functions to EXI and read them over serial. While this is possible in a few ways,
for the average user it's impractical and in many cases simply overkill.

Due to issues with Wii no Ma crashing at its earliest convenience, work started on a
tool that could load the channel with a shim to redirect output to NAND (or SD).
OSFatal/OSDumpContext internally call OSReport, allowing for easy crash log collecting,
among other debug logging given along the way. In a sense, it's a hope to have
Dolphin's HLE patching in hardware. We'll see if it can ever get there.

As it stands, this tool is far from done. In current state it fails to even load
the application, let alone patch in runtime.

## Contributing
Please ensure you have run `clang-format` on touched files where possible.


---
This repository is heavily based off of methods taken from usbloader-gx and
[wiilauncher](https://github.com/conanac/wiilauncher). Where parts are from is
commented where possible.

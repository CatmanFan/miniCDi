# miniCDi
An experimental Philips CD-i emulator for the Wii and 3DS. Currently only supports Mono-I and Mono-IV boards.

## To-Do
- [ ] Plane mixing/transparency emulation (VDSC)
- [ ] Pointer Devices

## Building
To compile, use devkitPro's `powerpc-eabi-cmake` (GC, (v)Wii, Wii U) or `arm-none-eabi-cmake` (3DS).

## Credits
Special credits to [Stovent](https://github.com/Stovent), [CD-i Fan](https://github.com/cdifan) and [Slamy](https://github.com/Slamy) for helping me on this project. The source code was partially based off of Stovent's implementations of the relevant components in [CeDImu](https://github.com/Stovent/CeDImu), as well as CD-i Fan's [cdichips](https://github.com/cdifan/cdichips) documentation of several components including the MCD212, SCC68070 (UART), IKAT and SLAVE. The emulator uses [Musashi](https://github.com/kstenerud/Musashi) version 4.10 as a core for the 68070 processor.
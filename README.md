# miniCDi
An experimental Philips CD-i emulator for the Wii and 3DS.
The current goal is for the emulator to display the player shell when provided with a CD-i 220 F2 system ROM.

## To-Do
- [ ] ICA/DCA processing (MCD212)

## Building
To compile, use devkitPro's `powerpc-eabi-cmake` (GC, Wii) or `arm-none-eabi-cmake` (3DS).

## Credits
Special credits to [Stovent](https://github.com/Stovent), [CD-i Fan](https://github.com/cdifan) and [Slamy](https://github.com/Slamy) for their assistance. This project includes a part of Stovent's [CeDImu](https://github.com/Stovent/CeDImu) source code, which I adapted and simplified for readability/performance purposes, and is also based on the documentation of several components including the MCD212, SCC68070 (UART) and CD-i Fan's [cdichips](https://github.com/cdifan/cdichips) repo.

*This readme is currently under construction*
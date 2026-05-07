# miniCDi
A very barebones and experimental Philips CD-i 220/20 F2 emulator for the Wii and 3DS. Runs on [Rocket68](https://github.com/habedi/rocket68), and it is currently only intended to run and display the CD-i 220 player shell.

To compile, use devkitPro's `powerpc-eabi-cmake` (GC, Wii) or `arm-none-eabi-cmake` (3DS).

Special credits to [Stovent](https://github.com/Stovent), [CD-i Fan](https://github.com/cdifan) and [Slamy](https://github.com/Slamy) for their assistance. This project includes a part of Stovent's [CeDImu](https://github.com/Stovent/CeDImu) source code, which I adapted and simplified for readability/performance purposes, and is also based on the documentation of several components including the MCD212, SCC68070 (UART) and CD-i Fan's [cdichips](https://github.com/cdifan/cdichips) repo.

*This readme is currently under construction*